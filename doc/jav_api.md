# jav Reflection Bridge API

The `jav` API allows Lua mods to dynamically access Java and Android APIs via reflection.

## API Reference

### `jav.import(className)`
Returns a proxy object for the specified Java class.
Example: `local Toast = jav.import("android.widget.Toast")`

### `jav.new(className, ...)`
Creates a new instance of a Java class.
Example: `local intent = jav.new("android.content.Intent", "android.intent.action.VIEW")`

### `jav.call(className, methodName, ...)`
Calls a static method on a Java class.

### `jav.get(fieldName)`
Gets the value of a static field (use dot notation for full path).
Example: `local sdk = jav.get("android.os.Build.VERSION.SDK_INT")`

### `jav.methods(target)`
Returns a list of available methods for a class or object.

### `jav.fields(target)`
Returns a list of available fields for a class or object.

### `jav.on(eventName, callback)`
Registers a listener for Android events.
Example: `jav.on("battery_changed", function(level) ... end)`

### `jav.async(workFunc, callbackFunc)`
Executes a function and calls the callback with the result.
