# Overview

Protocol 都应该含有以下定义:

- GUID - 全局唯一标识符
- Protocol Interface Structure - Protocol接口结构体
- Protocol Services - Protocol服务(接口函数)

## GUID

每一个 Protocol 有着惟一的 GUID,它将在操作 Protocol 时使用.

```c++
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  { \
    0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }
```

## Protocol Interface Structure

```c++
struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  ///
  /// The version of the EFI_SIMPLE_FILE_SYSTEM_PROTOCOL. The version
  /// specified by this specification is 0x00010000. All future revisions
  /// must be backwards compatible.
  ///
  UINT64                                      Revision;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
};
```

## Protocol Services

调用上述 OpenVolume:

```c++
typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
  IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *This,
  OUT EFI_FILE_PROTOCOL                 **Root
  );
```

This 指针区别于 C++,这里是需要手动传参,且这个指针是 **Protocol Interface Structure**

