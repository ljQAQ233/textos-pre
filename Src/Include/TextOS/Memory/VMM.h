#pragma once
/* Check if the Vrt is a canonical format
   vrt addr and adjust it if it is invalid

   @retval  int   The state */
int VMM_CaAdjust (u64 *Vrt);

/* Link virtual pages that Vrt points, 
   Num holds with an real physical pages */
void *VMM_PhyAuto (u64 Vrt, size_t Num, u16 Flgs);
