// IC10

include "clang/Basic/BuiltinsBase.td"

def IC10Prefix : NamePrefix<"__builtin_ic10_">;

class IC10Builtin<string prototype> : TargetBuiltin {
  let Spellings = [NAME];
  let Prototype = prototype;
  let RequiredNamePrefix = IC10Prefix;
}

def yield : IC10Builtin<"void()">;
def sleep : IC10Builtin<"void(double)">;
