#ifndef GRID_LUA_API_H_INCLUDED
#define GRID_LUA_API_H_INCLUDED

#include "sam.h"
#include "grid_module.h"

#include "thirdparty/Lua/lua-5.4.3/src/lua.h"
#include "thirdparty/Lua/lua-5.4.3/src/lualib.h"
#include "thirdparty/Lua/lua-5.4.3/src/lauxlib.h"


int _gettimeofday(void);
int _open(void);
int _times(void);
int _unlink(void);
int _link(void);

// GRID LOOKUP TABLE
#define GRID_LUA_GLUT_source \
"function glut (a, ...)      \
 local t = table.pack(...)   \
 for i = 1, t.n//2*2 do      \
  if i%2 == 1 then           \
   if t[i] == a then         \
    return t[i+1]            \
   end                       \
  end                        \
 end                         \
 return nil                  \
end"

// GRID LIMIT
#define GRID_LUA_GLIM_source  \
"function glim (a, min, max)  \
 if a>max then return max end \
 if a<min then return min end \
 return a \
end"

#define GRID_LUA_STDO_LENGTH    100
#define GRID_LUA_STDI_LENGTH    100

#define GRID_LUA_STDE_LENGTH    100

struct grid_lua_model{
    
    lua_State *L;

    uint32_t stdo_len;
    uint32_t stdi_len;

    uint32_t stde_len;

    uint8_t stdo[GRID_LUA_STDO_LENGTH];
    uint8_t stdi[GRID_LUA_STDI_LENGTH];

    uint8_t stde[GRID_LUA_STDE_LENGTH];

    uint32_t dostring_count;

};

struct grid_lua_model grid_lua_state;



uint8_t grid_lua_debug_memory_stats(struct grid_lua_model* mod, char* message);
void grid_lua_gc_try_collct(struct grid_lua_model* mod);

static int grid_lua_panic(lua_State *L);

uint8_t grid_lua_init(struct grid_lua_model* mod);
uint8_t grid_lua_deinit(struct grid_lua_model* mod);

uint8_t grid_lua_start_vm(struct grid_lua_model* mod);
uint8_t grid_lua_stop_vm(struct grid_lua_model* mod);

uint32_t grid_lua_dostring(struct grid_lua_model* mod, char* code);

void grid_lua_clear_stdi(struct grid_lua_model* mod);
void grid_lua_clear_stdo(struct grid_lua_model* mod);

void grid_lua_clear_stde(struct grid_lua_model* mod);

uint8_t grid_lua_ui_init(struct grid_lua_model* mod, struct grid_sys_model* sys);

uint8_t grid_lua_ui_init_po16(struct grid_lua_model* mod);
uint8_t grid_lua_ui_init_bu16(struct grid_lua_model* mod);
uint8_t grid_lua_ui_init_pbf4(struct grid_lua_model* mod);
uint8_t grid_lua_ui_init_en16(struct grid_lua_model* mod);







#endif