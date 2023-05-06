# Overview

如你所见啊，真的有简单的方法!

```c++
typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_PROTOCOL)(
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration, OPTIONAL // 可选项
  OUT VOID      **Interface
  );
```

它将返回第一个找到的 **Protocol实例**

这里以 `GraphicsOutpurProtocol` 为例

```c++
EFI_STATUS InitializeGraphicsServices ()
```

- `Src/SigmaBootPkg/Graphics.c`

# 缺点

当有多个Protocol实例时不能选择.
