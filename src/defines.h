#ifndef DEFINES_H
#define DEFINES_H

#define BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wc++98-compat-pedantic\"") \
    _Pragma("GCC diagnostic ignored \"-Wdisabled-macro-expansion\"") \
    _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
    _Pragma("GCC diagnostic ignored \"-Wpadded\"") \
    _Pragma("GCC diagnostic ignored \"-Wunreachable-code\"") \
    _Pragma("GCC diagnostic ignored \"-Wexit-time-destructors\"") \
    _Pragma("GCC diagnostic ignored \"-Wused-but-marked-unused\"") \
    _Pragma("GCC diagnostic ignored \"-Wshorten-64-to-32\"") \
    _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"") \
    _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
    _Pragma("GCC diagnostic ignored \"-Wswitch-enum\"") \
    _Pragma("GCC diagnostic ignored \"-Wweak-vtables\"") \
    _Pragma("GCC diagnostic ignored \"-Wdocumentation-unknown-command\"") \
    _Pragma("GCC diagnostic ignored \"-Wdocumentation\"")
#define END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS \
    _Pragma("GCC diagnostic pop")

#define QP_DEFINE_STATIC_LOCAL(type, name) \
    type *name(); \
    type *name() { \
    static type& name = *new type; \
    return &name; \
    }
#define QP_DEFINE_STATIC_LOCAL_WITH_ARGS(type, name, arguments) \
    type *name() { \
    static type& name = *new type arguments; \
    return &name; \
    }

#define QP_FOREACH (variable, container) \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wshadow\"") \
    Q_FOREACH(variable, container) \
    _Pragma("clang diagnostic pop") \

#if defined QP_FOR_MYSQL && defined QP_FOR_SQLITE
#error You must not define both QP_FOR_SQLITE and QP_FOR_MYSQL
#endif

#if !defined QP_FOR_MYSQL && !defined QP_FOR_SQLITE
#error You must either define QP_FOR_SQLITE or QP_FOR_MYSQL
#endif

#ifdef QP_FOR_SQLITE
#define QP_NO_LOCKS
#define QP_NO_TIMESTAMPS
#define QP_NO_USERMANAGEMENT
#endif

#ifdef QP_FOR_MYSQL
// Nothing to define
#endif

#endif // DEFINES_H
