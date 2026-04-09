// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

// Macros to mark classes and fields for the reflection generator (util/meta_codegen.py)

#define REFLECT_CLASS() \
	template<typename T, typename V> friend class DirectPropertyAccessor; \
	template<typename T, typename V> friend class LambdaPropertyAccessor; \
	friend class EngineRegistry;

#define REFLECT_FIELD()
#define REFLECT_METHOD()
