#include <cstdlib>
#include "memutils.h"
#include "sm_platform.h"
#include "igameevents.h"
#include "eiface.h"
#include "tier0/icommandline.h"

#include "sourcehook/sourcehook.h"
#include "sourcehook/sourcehook_impl.h"
#include "sourcehook/sourcehook_impl_chookidman.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef DEFAULT_TICK_INTERVAL // hl2sdk-l4d2 is wrong, too lazy to patch
#undef DEFAULT_TICK_INTERVAL
#endif
#define DEFAULT_TICK_INTERVAL 0.03333333
//---------------------------------------------------------------------------------
// Purpose: a sample 3rd party plugin class
//---------------------------------------------------------------------------------
class L4DTickRate: public IServerPluginCallbacks, public IGameEventListener
{
public:
	L4DTickRate();
	~L4DTickRate();

	// IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char     *GetPluginDescription( void );      
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args );
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

	// added with version 3 of the interface.
	virtual void			OnEdictAllocated( edict_t *edict );
	virtual void			OnEdictFreed( const edict_t *edict  );	

	// IGameEventListener Interface
	virtual void FireGameEvent( KeyValues * event );

	virtual int GetCommandIndex() { return m_iClientCommandIndex; }
private:
	int m_iClientCommandIndex;
};


//
L4DTickRate g_L4DTickRatePlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(L4DTickRate, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_L4DTickRatePlugin );

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
L4DTickRate::L4DTickRate()
{
	m_iClientCommandIndex = 0;
}

L4DTickRate::~L4DTickRate()
{
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void L4DTickRate::GameFrame( bool simulating )
{
}

SourceHook::Impl::CSourceHookImpl g_SourceHook;
SourceHook::ISourceHook *g_SHPtr = &g_SourceHook;
int g_PLID = 0;

SH_DECL_HOOK0(IServerGameDLL, GetTickInterval, const, 0, float);

float GetTickInterval()
{
	float tickinterval = DEFAULT_TICK_INTERVAL;

	if ( CommandLine()->CheckParm( "-tickrate" ) )
	{
		float tickrate = CommandLine()->ParmValue( "-tickrate", 0 );
		Msg("Tickrate_Enabler: Read TickRate %f\n", tickrate);
		if ( tickrate > 10 )
			tickinterval = 1.0f / tickrate;
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, tickinterval );
}

bool PatchBoomerVomit(IServerGameDLL * gamedll);

IServerGameDLL *gamedll = NULL;

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool L4DTickRate::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	char aServerGameDLL[] = "ServerGameDLL000";
	do
	{
		aServerGameDLL[15]++;
		gamedll = (IServerGameDLL*)gameServerFactory(aServerGameDLL,NULL);
	} while(aServerGameDLL[15] < '9' && !gamedll);
	if(!gamedll)
	{
		Warning("Tickrate_Enabler: Failed to get a pointer on ServerGameDLL.\n");
		return false;
	}
	Msg("Tickrate_Enabler: Found ServerGameDLL at %s\n", aServerGameDLL);

	SH_ADD_HOOK(IServerGameDLL, GetTickInterval, gamedll, SH_STATIC(GetTickInterval), false);

	struct DynLibInfo dlinfo;
	bool res = g_MemUtils.GetLibraryInfo(gamedll, dlinfo);
	if(res) Msg("Found dlinfo\n");
	else Msg("Not Found dlinfo\n");
	if(!PatchBoomerVomit(gamedll))
	{
		Warning("Tickrate_Enabler: Failed to patch boomer vomit behavior");
		return false;
	}
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void L4DTickRate::Unload( void )
{
	SH_REMOVE_HOOK(IServerGameDLL, GetTickInterval, gamedll, SH_STATIC(GetTickInterval), false);
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void L4DTickRate::Pause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void L4DTickRate::UnPause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *L4DTickRate::GetPluginDescription( void )
{
	return "Tickrate_Enabler 0.2, ProdigySim";
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void L4DTickRate::LevelInit( char const *pMapName )
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void L4DTickRate::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void L4DTickRate::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void L4DTickRate::ClientActive( edict_t *pEntity )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void L4DTickRate::ClientDisconnect( edict_t *pEntity )
{
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void L4DTickRate::ClientPutInServer( edict_t *pEntity, char const *playername )
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void L4DTickRate::SetCommandClient( int index )
{
	m_iClientCommandIndex = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void L4DTickRate::ClientSettingsChanged( edict_t *pEdict )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT L4DTickRate::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT L4DTickRate::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT L4DTickRate::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void L4DTickRate::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
}
void L4DTickRate::OnEdictAllocated( edict_t *edict )
{
}
void L4DTickRate::OnEdictFreed( const edict_t *edict  )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void L4DTickRate::FireGameEvent( KeyValues * event )
{
}


struct fakeGlobals {
	float padding[4];
	float frametime;
};

struct fakeGlobals g_FakeGlobals = { {0.0, 0.0, 0.0, 0.0}, DEFAULT_TICK_INTERVAL};
struct fakeGlobals *gp_FakeGlobals = &g_FakeGlobals;
void PatchGlobalsRead(void * readaddr)
{
#if defined PLATFORM_LINUX
	static unsigned char patch[]="\x8b\x00\x00\x00\x00\x90\x90\x90";
#elif defined PLATFORM_WINDOWS
	static unsigned char patch[]="\x00";
#endif
	*(void**)&patch[1] = gp_FakeGlobals;
	unsigned char * junk=(unsigned char*)readaddr;
	Msg("gp_fakeglobals: %p\n", gp_FakeGlobals);
	Msg("Patching %02x%02x%02x%02x%02x%02x%02x%02x to %02x%02x%02x%02x%02x%02x%02x%02x\n",
		(uint32)junk[0], (uint32)junk[1], (uint32)junk[2], (uint32)junk[3], 
		(uint32)junk[4], (uint32)junk[5], (uint32)junk[6], (uint32)junk[7],
		(uint32)patch[0], (uint32)patch[1], (uint32)patch[2], (uint32)patch[3], 
		(uint32)patch[4], (uint32)patch[5], (uint32)patch[6], (uint32)patch[7]);
	g_MemUtils.SetMemPatchable(readaddr, 8);
	memcpy(readaddr, patch, 8);
}
bool PatchBoomerVomit(IServerGameDLL * gamedll)
{
	void * p_CVomitUpdateAbility = NULL;

#if defined PLATFORM_LINUX
    const char CVomitUpdateAbility_Symbol[] = "_ZN6CVomit13UpdateAbilityEv";
	Dl_info info;
    /* GNU only: returns 0 on error, inconsistent! >:[ */
    if (dladdr(gamedll, &info) != 0)
    {
    	void *handle = dlopen(info.dli_fname, RTLD_NOW);
        if (handle)
        {
			p_CVomitUpdateAbility = g_MemUtils.ResolveSymbol(handle, CVomitUpdateAbility_Symbol);
        	dlclose(handle);
        } else {
			Warning("Nohandle!\n");
			return false;
		}
	}
	else
	{
		Warning("No DLINFO!\n");
		return false;
	}

#elif defined PLATFORM_WINDOWS
	// Pattern to find CVomitUpdateAblity
	const char CVomitUpdateAbility_pattern[] = "STUPID_PATTERN_FOR_THAT_FUNCTION";
	p_CVomitUpdateAbility = g_MemUtils.FindPattern(gamedll, CVomitUpdateAbility_pattern, sizeof(CVomitUpdateAbility_pattern));
#else
	What platform is this?
#endif
	if(!p_CVomitUpdateAbility)
	{
		Warning("Unable to find CVomitUpdateAbility\n");
		return false;
	}
	Msg("CVomitUpdateAbility at %p\n", p_CVomitUpdateAbility);

	void * end = (void *)(((char *)p_CVomitUpdateAbility) + 0x500);

#if defined PLATFORM_LINUX
	// mov e?x, ebp+gpGlobalsOffset
	const char globReadPattern[] = "\x8B\x8B\xfc\xf4\xff\xff\x8b";
#elif defined PLATFORM_WINDOWS
	const char globReadPattern[] = "\x00";
#endif

	Msg("Searching for from %p to %p for %d bytes\n", p_CVomitUpdateAbility, end, sizeof(globReadPattern)-1);
	int patchcnt=0;
	while((p_CVomitUpdateAbility = g_MemUtils.FindPattern(p_CVomitUpdateAbility, 
			end, globReadPattern, sizeof(globReadPattern)-1)) != NULL)
	{
		++patchcnt;
		Msg("Found something at %p\n", p_CVomitUpdateAbility);
		unsigned char * test = (unsigned char *)p_CVomitUpdateAbility;
//		Msg("It's %02x %02x %p\n", (uint32)test[0], (uint32)test[1], *(void **)(test+2));
		PatchGlobalsRead(p_CVomitUpdateAbility);
		p_CVomitUpdateAbility=(void *)(((char*)p_CVomitUpdateAbility)+1);
		Msg("Searching for from %p to %p for %d bytes\n", p_CVomitUpdateAbility, end, sizeof(globReadPattern)-1);
	};
	Msg("Found %d instances\n", patchcnt);

	return true;
}

