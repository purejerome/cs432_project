Decaf Scope & Duplicate Edge-Case Tests
=======================================

Naming convention:
- *_ok.decaf           => should compile / pass static analysis (re: scoping/duplicates)
- *_error.decaf        => should produce a static-analysis error (duplicate / undefined / not-a-function)

Error categories represented:
- duplicate in same scope (globals, params, locals)
- undefined identifier (using a block-local outside its scope)
- not-a-function (local/param shadows a global function, then attempts a call)
- function overloading / mixed-kind duplicate at global scope

These files assume Decaf is case-sensitive and supports:
- `def` function definitions
- `void`, `int`, `bool`
- arrays: `int[10] a;`
- blocks with `{ ... }`
- `if ( ... ) { ... } else { ... }`
- `while ( ... ) { ... }`
- boolean literals: `true`, `false`
- `return;` in void functions, `return <expr>;` in non-void functions
