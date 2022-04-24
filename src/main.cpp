#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include <tier0/dbg.h>
#include <cstdio>
#include <GarrysMod/Lua/Interface.h>
#include <Platform.hpp>
#if ARCHITECTURE_IS_X86
#include <Color.h>
#elif ARCHITECTURE_IS_X86_64
#include <color.h>
#endif

lua_State* luaState = NULL;
volatile bool inspew = false; // volatile not needed but itll remind me

// BELOW IS THE OLD ENGINE SPEW THAT ONLY WORKS WITH THE OLD SPEWING SYSTEM
// WHICH IS NOT THE SAME AS the X64 BRANCH OF GMOD
#if ARCHITECTURE_IS_X86

SpewOutputFunc_t g_fnOldSpew = NULL;

// which thread
#ifdef _WIN32
DWORD g_iMainThread;
#else
pthread_t g_iMainThread;
#endif


SpewRetval_t LuaSpew(SpewType_t spewType, const char* pMsg)
{
	// test for wrong thread...
#ifdef _WIN32
	if (GetCurrentThreadId() != g_iMainThread)
#else
	if (pthread_self() != g_iMainThread)
#endif
		return g_fnOldSpew(spewType, pMsg);

	// no message...
	if (!pMsg || inspew || !luaState)
		return g_fnOldSpew(spewType, pMsg);

	// NOW WE ARE TALKING
	GarrysMod::Lua::ILuaBase* LUA = luaState->luabase;

	inspew = true;

	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->GetField(-1, "hook");
	LUA->GetField(-1, "Run");

	LUA->PushString("EngineSpew"); // MAY TRIGGER LUA PANIC
	LUA->PushNumber((double)spewType);
	LUA->PushString(pMsg);
	LUA->PushString(GetSpewOutputGroup());
	LUA->PushNumber((double)GetSpewOutputLevel());

	// screw tables. Lets just push this in like this and be done with it.
	const Color* c = GetSpewOutputColor();

	int Red, Green, Blue, Alpha = 0;
	c->GetColor(Red, Green, Blue, Alpha);

	LUA->PushNumber((float)Red);
	LUA->PushNumber((float)Green);
	LUA->PushNumber((float)Blue);

	/*
		lua_run hook.Add("EngineSpew","a",function(t,msg,grp,lev,r,g,b) print(...) end)
	*/


	// hook.Run("EngineSpew",0,"","",0,0,0,0);
	if (LUA->PCall(8, 1, 0) != 0)
	{
		Warning("[EngineSpew error] %s\n", LUA->GetString());
		LUA->Pop(3);
		inspew = false;
		return g_fnOldSpew(spewType, pMsg);
	}

	if (LUA->IsType(-1, GarrysMod::Lua::Type::BOOL) && LUA->GetBool())
	{
		LUA->Pop(3);
		inspew = false;
		return SPEW_CONTINUE;
	}

	LUA->Pop(3);

	inspew = false;

	return g_fnOldSpew(spewType, pMsg);
}

void spew_hook()
{
	g_fnOldSpew = GetSpewOutputFunc();
	SpewOutputFunc(LuaSpew);
}

GMOD_MODULE_OPEN()
{
	luaState = LUA->GetState();

#ifdef _WIN32
	g_iMainThread = GetCurrentThreadId();
#else
	g_iMainThread = pthread_self();
#endif

	spew_hook();

	return 0;
}

GMOD_MODULE_CLOSE()
{
	luaState = NULL;
	SpewOutputFunc(g_fnOldSpew);
	return 0;
}

// NEW LOGGING SYSTEM AVAILABLE IN THE X64 BRANCH AND POTENTIALLY FOR THE FORSEEABLE FUTURE
#elif ARCHITECTURE_IS_X86_64

class SpewListener : public ILoggingListener
{
public:
	SpewListener(bool bQuietPrintf = false, bool bQuietDebugger = false) {}

	void Log(const LoggingContext_t* pContext, const char* pMsg) override
	{
		if (inspew || !luaState) return;

		const CLoggingSystem::LoggingChannel_t* chan = LoggingSystem_GetChannel(pContext->m_ChannelID);
		GarrysMod::Lua::ILuaBase* LUA = luaState->luabase;

		inspew = true;

		LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "hook");
		LUA->GetField(-1, "Run");

		LUA->PushString("EngineSpew"); // MAY TRIGGER LUA PANIC
		LUA->PushNumber((double)pContext->m_Flags);
		LUA->PushString(pMsg);
		LUA->PushString(chan->m_Name);
		LUA->PushNumber((double)pContext->m_Severity);

		// screw tables. Lets just push this in like this and be done with it.
		const Color* c = &pContext->m_Color;

		int Red, Green, Blue, Alpha = 0;
		c->GetColor(Red, Green, Blue, Alpha);

		LUA->PushNumber((float)Red);
		LUA->PushNumber((float)Green);
		LUA->PushNumber((float)Blue);

		if (LUA->PCall(8, 1, 0) != 0)
		{
			Warning("[EngineSpew error] %s\n", LUA->GetString());
		}

		LUA->Pop(3);
		inspew = false;
	}
};

ILoggingListener* listener = new SpewListener();
GMOD_MODULE_OPEN()
{
	luaState = LUA->GetState();
	LoggingSystem_PushLoggingState(false, false);
	LoggingSystem_RegisterLoggingListener(listener);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	LoggingSystem_UnregisterLoggingListener(listener);
	LoggingSystem_PopLoggingState(false);
	luaState = NULL;
	
	return 0;
}
#else
#error Not x86 or x64 ?????
#endif
