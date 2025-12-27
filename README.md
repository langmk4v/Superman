# Fire-language
個人的に趣味で開発しているプログラミング言語です。<br>
Rust の構文に似ている静的型付けで、実行時コンパイル機能を搭載したインタプリタ言語。

# Language Documentation
[`日本語`](./Document.md) <br>
[`ENGLISH`](./DocumentEnglish.md) <br>

# Build Step
## Requirements
リンカに mold を使用しているので、ない場合はインストールしてください <br>
https://github.com/rui314/mold

## Compile
```bash
mkdir build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" -B build
cmake --build build -j${nproc}
```

## Run your program.
- Create source file as extension `.fire` .
```
fn main() -> int {
    println("Hello, World!");
}
```

- Let's Execute.
```
./build/fire main.fire
```

# Discord Server
https://discord.gg/CURgAtGH8Z
