#pragma once
#include "l_base.h"

class ModApiVisuals : public ModApiBase
{
private:
	static int l_add_post_processing_pass(lua_State *L);
	static int l_remove_post_processing_pass(lua_State *L);
	static int l_set_post_processing_uniform(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};
