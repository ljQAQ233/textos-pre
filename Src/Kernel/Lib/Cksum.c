/* Check sum library */

bool CkSum (void *Data, size_t Len)
{
    u8 Ckr = 0;
    
    while (Len--)
        Ckr += *(u8 *)Data++;

    return Ckr == 0;
}

u8 MakeCksum (void *Data, size_t Len)
{
    u8 Ckr = 0;

    while (Len--)
        Ckr += *(u8 *)Data++;

    return 0 - Ckr;
}

