mac brew 安装 nasm：

```command
brew install nasm
```

编译：

```command
nasm -f macho64 -o exam.o exam.asm
```

链接动态库：
```command
ld -macosx_version_min 11.3 -o exam -e _MAIN exam.o -L /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem
```

