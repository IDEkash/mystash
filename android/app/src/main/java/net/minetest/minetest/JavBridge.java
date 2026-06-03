package net.minetest.minetest;

import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.Keep;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@Keep
@SuppressWarnings("unused")
public class JavBridge {
    private static final String TAG = "JavBridge";
    private static final Map<String, Class<?>> classCache = new ConcurrentHashMap<>();
    private static final Map<String, Method> methodCache = new ConcurrentHashMap<>();
    private static final Map<String, Field> fieldCache = new ConcurrentHashMap<>();
    private static final Map<String, Constructor<?>> constructorCache = new ConcurrentHashMap<>();

    private static final ExecutorService executor = Executors.newCachedThreadPool();
    private static final Handler mainHandler = new Handler(Looper.getMainLooper());

    private static final List<AsyncResult> asyncResults = new ArrayList<>();
    private static final List<JavEvent> events = new ArrayList<>();

    public static class AsyncResult {
        public long callbackId;
        public Object result;
        public String error;

        public AsyncResult(long callbackId, Object result, String error) {
            this.callbackId = callbackId;
            this.result = result;
            this.error = error;
        }
    }

    public static class JavEvent {
        public String name;
        public Object data;

        public JavEvent(String name, Object data) {
            this.name = name;
            this.data = data;
        }
    }

    // --- Core API ---

    public static Class<?> importClass(String className) {
        try {
            return getCachedClass(className);
        } catch (Exception e) {
            Log.e(TAG, "importClass failed: " + className, e);
            return null;
        }
    }

    public static Object newInstance(String className, Object[] args) {
        try {
            Class<?> clazz = getCachedClass(className);
            Constructor<?> constructor = getBestConstructor(clazz, args);
            return constructor.newInstance(args);
        } catch (Exception e) {
            Log.e(TAG, "newInstance failed: " + className, e);
            throw new RuntimeException(e);
        }
    }

    public static Object callStatic(Class<?> clazz, String methodName, Object[] args) {
        try {
            Method method = getCachedMethod(clazz, methodName, args, true);
            return method.invoke(null, args);
        } catch (Exception e) {
            Log.e(TAG, "callStatic failed: " + clazz.getName() + "." + methodName, e);
            throw new RuntimeException(e);
        }
    }

    public static Object callInstance(Object instance, String methodName, Object[] args) {
        try {
            Method method = getCachedMethod(instance.getClass(), methodName, args, false);
            return method.invoke(instance, args);
        } catch (Exception e) {
            Log.e(TAG, "callInstance failed: " + instance.getClass().getName() + "." + methodName, e);
            throw new RuntimeException(e);
        }
    }

    public static Object getField(Object target, String fieldName) {
        try {
            Field field;
            if (target instanceof Class) {
                field = getCachedField((Class<?>) target, fieldName);
                return field.get(null);
            } else {
                field = getCachedField(target.getClass(), fieldName);
                return field.get(target);
            }
        } catch (Exception e) {
            Log.e(TAG, "getField failed: " + fieldName, e);
            throw new RuntimeException(e);
        }
    }

    public static void setField(Object target, String fieldName, Object value) {
        try {
            Field field;
            if (target instanceof Class) {
                field = getCachedField((Class<?>) target, fieldName);
                field.set(null, value);
            } else {
                field = getCachedField(target.getClass(), fieldName);
                field.set(target, value);
            }
        } catch (Exception e) {
            Log.e(TAG, "setField failed: " + fieldName, e);
            throw new RuntimeException(e);
        }
    }

    // --- Discovery Helpers ---

    public static String[] methods(Object target) {
        try {
            Class<?> clazz = (target instanceof Class) ? (Class<?>) target : target.getClass();
            Method[] methods = clazz.getMethods();
            String[] names = new String[methods.length];
            for (int i = 0; i < methods.length; i++) {
                names[i] = methods[i].getName();
            }
            return names;
        } catch (Exception e) {
            return new String[0];
        }
    }

    public static String[] fields(Object target) {
        try {
            Class<?> clazz = (target instanceof Class) ? (Class<?>) target : target.getClass();
            Field[] fields = clazz.getFields();
            String[] names = new String[fields.length];
            for (int i = 0; i < fields.length; i++) {
                names[i] = fields[i].getName();
            }
            return names;
        } catch (Exception e) {
            return new String[0];
        }
    }

    // --- Internal Helpers ---

    private static Class<?> getCachedClass(String className) throws ClassNotFoundException {
        Class<?> clazz = classCache.get(className);
        if (clazz == null) {
            clazz = Class.forName(className);
            classCache.put(className, clazz);
        }
        return clazz;
    }

    private static Method getCachedMethod(Class<?> clazz, String methodName, Object[] args, boolean isStatic) throws NoSuchMethodException {
        Class<?>[] argTypes = getTypes(args);
        String cacheKey = clazz.getName() + "." + methodName + "(" + typesToString(argTypes) + ")" + (isStatic ? ":static" : "");
        Method method = methodCache.get(cacheKey);
        if (method == null) {
            method = findBestMethod(clazz, methodName, argTypes, isStatic);
            methodCache.put(cacheKey, method);
        }
        return method;
    }

    private static String typesToString(Class<?>[] types) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < types.length; i++) {
            sb.append(types[i].getName());
            if (i < types.length - 1) sb.append(",");
        }
        return sb.toString();
    }

    private static Method findBestMethod(Class<?> clazz, String methodName, Class<?>[] argTypes, boolean isStatic) throws NoSuchMethodException {
        for (Method m : clazz.getMethods()) {
            if (m.getName().equals(methodName) && (Modifier.isStatic(m.getModifiers()) == isStatic)) {
                Class<?>[] params = m.getParameterTypes();
                if (params.length == argTypes.length) {
                    boolean match = true;
                    for (int i = 0; i < params.length; i++) {
                        if (!isAssignable(params[i], argTypes[i])) {
                            match = false;
                            break;
                        }
                    }
                    if (match) return m;
                }
            }
        }
        throw new NoSuchMethodException(methodName);
    }

    private static Constructor<?> getBestConstructor(Class<?> clazz, Object[] args) throws NoSuchMethodException {
        Class<?>[] argTypes = getTypes(args);
        for (Constructor<?> c : clazz.getConstructors()) {
            Class<?>[] params = c.getParameterTypes();
            if (params.length == argTypes.length) {
                boolean match = true;
                for (int i = 0; i < params.length; i++) {
                    if (!isAssignable(params[i], argTypes[i])) {
                        match = false;
                        break;
                    }
                }
                if (match) return c;
            }
        }
        throw new NoSuchMethodException("Constructor not found for " + clazz.getName());
    }

    private static Class<?>[] getTypes(Object[] args) {
        if (args == null) return new Class[0];
        Class<?>[] types = new Class[args.length];
        for (int i = 0; i < args.length; i++) {
            types[i] = args[i] != null ? args[i].getClass() : Object.class;
        }
        return types;
    }

    private static Field getCachedField(Class<?> clazz, String fieldName) throws NoSuchFieldException {
        String cacheKey = clazz.getName() + "#" + fieldName;
        Field field = fieldCache.get(cacheKey);
        if (field == null) {
            field = clazz.getField(fieldName);
            fieldCache.put(cacheKey, field);
        }
        return field;
    }

    private static boolean isAssignable(Class<?> target, Class<?> source) {
        if (source == null || target.isAssignableFrom(source)) return true;
        if (target.isPrimitive()) {
            if (target == int.class && (source == Integer.class || source == Double.class || source == Float.class || source == Long.class)) return true;
            if (target == long.class && (source == Long.class || source == Integer.class)) return true;
            if (target == double.class && (source == Double.class || source == Float.class)) return true;
            if (target == float.class && (source == Float.class || source == Double.class)) return true;
            if (target == boolean.class && source == Boolean.class) return true;
            if (target == char.class && source == Character.class) return true;
            if (target == byte.class && source == Byte.class) return true;
            if (target == short.class && source == Short.class) return true;
        }
        return false;
    }

    // --- Async & Events ---

    public static void asyncCall(final long callbackId, final Runnable runnable) {
        executor.execute(() -> {
            try {
                runnable.run();
            } catch (Exception e) {
                Log.e(TAG, "Async call failed", e);
                synchronized (asyncResults) {
                    asyncResults.add(new AsyncResult(callbackId, null, e.toString()));
                }
            }
        });
    }

    public static void postAsyncResult(long callbackId, Object result) {
        synchronized (asyncResults) {
            asyncResults.add(new AsyncResult(callbackId, result, null));
        }
    }

    public static void postEvent(String name, Object data) {
        synchronized (events) {
            events.add(new JavEvent(name, data));
        }
    }

    public static Object[] pollAsyncResults() {
        synchronized (asyncResults) {
            if (asyncResults.isEmpty()) return null;
            Object[] result = asyncResults.toArray();
            asyncResults.clear();
            return result;
        }
    }

    public static Object[] pollEvents() {
        synchronized (events) {
            if (events.isEmpty()) return null;
            Object[] result = events.toArray();
            events.clear();
            return result;
        }
    }
}
