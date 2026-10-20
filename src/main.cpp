#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include <cstdint>
#include <GarrysMod/Lua/Interface.h>
#include <GarrysMod/ModuleLoader.hpp>
#include <color.h>

namespace Tier0
{
	typedef int32_t SpewRetval_t;
	typedef SpewRetval_t (*SpewOutputFunc_t)(int32_t spewType, const char* pMsg);

	const SpewRetval_t SPEW_CONTINUE = 1;

	struct LoggingContext_t
	{
		int32_t m_ChannelID;
		int32_t m_Flags;
		int32_t m_Severity;
		Color m_Color;
	};

	struct LoggingChannel_t
	{
		int32_t m_ID;
		int32_t m_Flags;
		int32_t m_MinimumSeverity;
		Color m_SpewColor;
		char m_Name[32];
		void* m_pFirstTag;
	};

	class ILoggingListener
	{
	public:
		virtual void Log(const LoggingContext_t* pContext, const char* pMessage) = 0;
	};

	static void (*Warning)(const char* pMsg, ...) = nullptr;

	static void (*SpewOutputFunc)(SpewOutputFunc_t func) = nullptr;
	static SpewOutputFunc_t (*GetSpewOutputFunc)() = nullptr;
	static const char* (*GetSpewOutputGroup)() = nullptr;
	static int (*GetSpewOutputLevel)() = nullptr;
	static const Color* (*GetSpewOutputColor)() = nullptr;

	static void (*LoggingSystem_RegisterLoggingListener)(ILoggingListener* pListener) = nullptr;
	static void (*LoggingSystem_UnregisterLoggingListener)(ILoggingListener* pListener) = nullptr;
	static void (*LoggingSystem_PushLoggingState)(bool bThreadLocal, bool bClearState) = nullptr;
	static void (*LoggingSystem_PopLoggingState)(bool bThreadLocal) = nullptr;
	static const LoggingChannel_t* (*LoggingSystem_GetChannel)(int32_t channelID) = nullptr;
}

lua_State* luaState = NULL;
volatile bool inspew = false; // volatile not needed but itll remind me

// which thread
#ifdef _WIN32
DWORD g_iMainThread;
#else
pthread_t g_iMainThread;
#endif

static bool IsMainThread()
{
#ifdef _WIN32
	return GetCurrentThreadId() == g_iMainThread;
#else
	return pthread_equal(pthread_self(), g_iMainThread) != 0;
#endif
}

static bool RunEngineSpewHook(double type, const char* pMsg, const char* group, double level, const Color* c)
{
	GarrysMod::Lua::ILuaBase* LUA = luaState->luabase;

	inspew = true;

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");
	LUA->GetField(-1, "Run");

	LUA->PushString("EngineSpew"); // MAY TRIGGER LUA PANIC
	LUA->PushNumber(type);
	LUA->PushString(pMsg);
	LUA->PushString(group);
	LUA->PushNumber(level);

	// screw tables. Lets just push this in like this and be done with it.
	int Red, Green, Blue, Alpha = 0;
	c->GetColor(Red, Green, Blue, Alpha);

	LUA->PushNumber((float)Red);
	LUA->PushNumber((float)Green);
	LUA->PushNumber((float)Blue);

	/*
		lua_run hook.Add("EngineSpew","a",function(t,msg,grp,lev,r,g,b) print(...) end)
	*/

	// hook.Run("EngineSpew",0,"","",0,0,0,0);
	bool suppress = false;
	if (LUA->PCall(8, 1, 0) != 0)
	{
		if (Tier0::Warning != nullptr)
			Tier0::Warning("[EngineSpew error] %s\n", LUA->GetString());
	}
	else
	{
		suppress = LUA->IsType(-1, GarrysMod::Lua::Type::Bool) && LUA->GetBool();
	}

	LUA->Pop(3);
	inspew = false;

	return suppress;
}

Tier0::SpewOutputFunc_t g_fnOldSpew = NULL;

Tier0::SpewRetval_t LuaSpew(int32_t spewType, const char* pMsg)
{
	if (!pMsg || inspew || !luaState || !IsMainThread())
		return g_fnOldSpew(spewType, pMsg);

	bool suppress = RunEngineSpewHook(
		(double)spewType,
		pMsg,
		Tier0::GetSpewOutputGroup(),
		(double)Tier0::GetSpewOutputLevel(),
		Tier0::GetSpewOutputColor()
	);

	if (suppress)
		return Tier0::SPEW_CONTINUE;

	return g_fnOldSpew(spewType, pMsg);
}

class SpewListener : public Tier0::ILoggingListener
{
public:
	void Log(const Tier0::LoggingContext_t* pContext, const char* pMsg) override
	{
		if (!pMsg || inspew || !luaState || !IsMainThread()) return;

		const Tier0::LoggingChannel_t* chan = Tier0::LoggingSystem_GetChannel(pContext->m_ChannelID);
		RunEngineSpewHook(
			(double)pContext->m_Flags,
			pMsg,
			chan->m_Name,
			(double)pContext->m_Severity,
			&pContext->m_Color
		);
	}
};

static SpewListener listener;
static bool useLoggingSystem = false;

template<typename T>
static bool ResolveSymbol(const SourceSDK::ModuleLoader& module, T& out, const char* name)
{
	out = reinterpret_cast<T>(module.GetSymbol(name));
	return out != nullptr;
}

static bool ResolveLoggingSystem(const SourceSDK::ModuleLoader& tier0)
{
	return
		ResolveSymbol(tier0, Tier0::LoggingSystem_RegisterLoggingListener, "LoggingSystem_RegisterLoggingListener") &&
		ResolveSymbol(tier0, Tier0::LoggingSystem_UnregisterLoggingListener, "LoggingSystem_UnregisterLoggingListener") &&
		ResolveSymbol(tier0, Tier0::LoggingSystem_PushLoggingState, "LoggingSystem_PushLoggingState") &&
		ResolveSymbol(tier0, Tier0::LoggingSystem_PopLoggingState, "LoggingSystem_PopLoggingState") &&
		ResolveSymbol(tier0, Tier0::LoggingSystem_GetChannel, "LoggingSystem_GetChannel");
}

static bool ResolveSpew(const SourceSDK::ModuleLoader& tier0)
{
	return
		ResolveSymbol(tier0, Tier0::SpewOutputFunc, "SpewOutputFunc") &&
		ResolveSymbol(tier0, Tier0::GetSpewOutputFunc, "GetSpewOutputFunc") &&
		ResolveSymbol(tier0, Tier0::GetSpewOutputGroup, "GetSpewOutputGroup") &&
		ResolveSymbol(tier0, Tier0::GetSpewOutputLevel, "GetSpewOutputLevel") &&
		ResolveSymbol(tier0, Tier0::GetSpewOutputColor, "GetSpewOutputColor");
}

GMOD_MODULE_OPEN()
{
	{
		SourceSDK::ModuleLoader tier0("tier0");
		ResolveSymbol(tier0, Tier0::Warning, "Warning");
		useLoggingSystem = ResolveLoggingSystem(tier0);
		if (!useLoggingSystem && !ResolveSpew(tier0))
			LUA->ThrowError("failed to find a console output hook in tier0");
	}

	luaState = LUA->GetState();

#ifdef _WIN32
	g_iMainThread = GetCurrentThreadId();
#else
	g_iMainThread = pthread_self();
#endif

	if (useLoggingSystem)
	{
		Tier0::LoggingSystem_PushLoggingState(false, false);
		Tier0::LoggingSystem_RegisterLoggingListener(&listener);
	}
	else
	{
		g_fnOldSpew = Tier0::GetSpewOutputFunc();
		Tier0::SpewOutputFunc(LuaSpew);
	}

	return 0;
}

GMOD_MODULE_CLOSE()
{
	if (useLoggingSystem)
	{
		Tier0::LoggingSystem_UnregisterLoggingListener(&listener);
		Tier0::LoggingSystem_PopLoggingState(false);
	}
	else if (g_fnOldSpew != NULL)
	{
		Tier0::SpewOutputFunc(g_fnOldSpew);
	}

	luaState = NULL;

	return 0;
}
