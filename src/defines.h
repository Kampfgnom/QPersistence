#ifndef DEFINES_H
#define DEFINES_H

#define BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wc++98-compat-pedantic\"") \
    _Pragma("clang diagnostic ignored \"-Wdisabled-macro-expansion\"") \
    _Pragma("clang diagnostic ignored \"-Wsign-conversion\"") \
    _Pragma("clang diagnostic ignored \"-Wpadded\"") \
    _Pragma("clang diagnostic ignored \"-Wunreachable-code\"") \
    _Pragma("clang diagnostic ignored \"-Wexit-time-destructors\"") \
    _Pragma("clang diagnostic ignored \"-Wused-but-marked-unused\"") \
    _Pragma("clang diagnostic ignored \"-Wshorten-64-to-32\"") \
    _Pragma("clang diagnostic ignored \"-Wfloat-equal\"") \
    _Pragma("clang diagnostic ignored \"-Wconversion\"") \
    _Pragma("clang diagnostic ignored \"-Wswitch-enum\"") \
    _Pragma("clang diagnostic ignored \"-Wweak-vtables\"") \
    _Pragma("clang diagnostic ignored \"-Wdocumentation-unknown-command\"") \
    _Pragma("clang diagnostic ignored \"-Wdocumentation\"")
#define END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS \
    _Pragma("clang diagnostic pop")

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

#define foreach(variable, container) \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wshadow\"") \
    Q_FOREACH(variable, container) \
    _Pragma("clang diagnostic pop") \

#endif // DEFINES_H
