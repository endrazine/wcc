
/*
** ===============================================================
** some useful macros
** ===============================================================
*/


#define luaL_newlibtable(L,l)   \
  lua_createtable(L, 0, sizeof(l)/sizeof((l)[0]) - 1)

#define luaL_newlib(L,l)  \
  (luaL_checkversion(L), luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))

#define luaL_argcheck(L, cond,arg,extramsg)     \
                ((void)((cond) || luaL_argerror(L, (arg), (extramsg))))
#define luaL_checkstring(L,n)   (luaL_checklstring(L, (n), NULL))
#define luaL_optstring(L,n,d)   (luaL_optlstring(L, (n), (d), NULL))

#define luaL_typename(L,i)      lua_typename(L, lua_type(L,(i)))

#define luaL_dofile(L, fn) \
        (luaL_loadfile(L, fn) || lua_pcall(L, 0, LUA_MULTRET, 0))

#define luaL_dostring(L, s) \
        (luaL_loadstring(L, s) || lua_pcall(L, 0, LUA_MULTRET, 0))

#define luaL_getmetatable(L,n)  (lua_getfield(L, LUA_REGISTRYINDEX, (n)))

#define luaL_opt(L,f,n,d)       (lua_isnoneornil(L,(n)) ? (d) : f(L,(n)))

#define luaL_loadbuffer(L,s,sz,n)       luaL_loadbufferx(L,s,sz,n,NULL)


