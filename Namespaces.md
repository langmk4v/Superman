```cpp
namespace fire {
  namespace lexer {
    struct Token;
    class Lexer;
  }

  namespace parser {
    struct Node;
    
    struct Nd* : public Node;  // NdValue, NdSymbol, ...

    class Parser;
  }

  namespace sema {
    struct TypeInfo;

    struct Symbol;
    struct SymbolTable;

    struct ScopeContext;
    struct UnnamedScope;
    struct FunctionScope;
    struct EnumScope;
    struct ClassScope;
    struct ModuleScope;

    struct ExprType;
    struct SymbolFindResult;

    class Sema;
  }

  namespace vm {
    enum Operations : uint8_t;
    struct Instruction; // 一つのアセンブリ命令を表す

    namespace compiler {
      // Compiler: Node* から Instruction を作成
      class Compiler;
    }

    namespace interp {
      enum class BuiltinFuncID : uint32_t; // ビルトイン関数の ID
      struct Object;  // オブジェクト
      struct VCPU;    // Virtual CPU
      class Interp;   // インタプリタ ( Instruction の配列を実行する )
    }
  }
}
```