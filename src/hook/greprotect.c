#include <ntddk.h>
#include <undoc.h>
#include <utils.h>
#include <var.h>
#include <wdm.h>
#include <stdint.h>

uint64_t hooked_GreProtectSpriteContent(uint64_t gre_context, PVOID hwnd, int use_dwm_path, uint8_t new_affinity)
{
    // this doesnt update the value for GetWindowAffinity - this is the raw rendering function (formats and sends a DWM LPC packet).
    // so if you SetWindowDisplayAffinity(WDA_EXCLUDEFROMCAPTURE), with this hook, it does nothing, and GetDisplayAffinity returns WDA_EXCLUDEFROMCAPTURE.
    // reference: https://github.com/KANKOSHEV/NoScreen
    print("[hooked_GreProtectSpriteContent] someone tried to set affinity on (window handle=%d) to 0x%X, returning 1.\n", (uint64_t)hwnd, new_affinity);
    return 1;

    //     int want_hidden = new_affinity & 1;
    //     int want_hidden_transparent = (new_affinity & WDA_EXCLUDEFROMCAPTURE) == WDA_EXCLUDEFROMCAPTURE;

    //     int32_t redirection_style = 0;
    //     uint32_t redirection_param1 = 0;
    //     uint32_t redirection_param2 = 0;
    //     uint32_t found_live_sprite = 0;

    //     void* dwm_globals = Gre_Base_Globals(NULL);

    //     if (!use_dwm_path || !IsDwmActive()) return found_live_sprite;

    //     // Enter DWM critical section
    //     DwmCritScope crit;
    //     DwmCritScope_enter(&crit, dwm_globals, gre_context, 0);

    //     if (!IsDwmActive()) // double-check after acquiring the lock
    //         goto leave_crit;

    //     // Resolve the DWM sprite for hwnd
    //     DwmSpriteRef sprite_ref;
    //     DwmSpriteRef_init(&sprite_ref, hwnd);
    //     void* sprite = sprite_ref.sprite;

    //     if (sprite != NULL)
    //     {
    //         void* surface = SPRITE_SURFACE(sprite);

    //         if (surface != NULL)
    //         {
    //             uint32_t cur_flags = SPRITE_FLAGS(sprite);
    //             int currently_hidden = (cur_flags & SPRITE_HIDDEN) != 0;
    //             int currently_hidden_transp = (cur_flags & SPRITE_HIDDEN_TRANSPARENT) != 0;

    //             found_live_sprite = 1;

    //             // Only push an update to DWM if the protection state is actually changing
    //             if (currently_hidden != want_hidden || currently_hidden_transp != want_hidden_transparent)
    //             {
    //                 // Update sprite flags, touching only bits 3 and 6.
    //                 // Equivalent to the original XOR/mask expression; preserves all other bits.

    //                 uint32_t new_flags = cur_flags;
    //                 new_flags = (new_flags & ~SPRITE_HIDDEN) | (want_hidden ? SPRITE_HIDDEN : 0u);
    //                 new_flags = (new_flags & ~SPRITE_HIDDEN_TRANSPARENT) | (want_hidden_transparent ? SPRITE_HIDDEN_TRANSPARENT : 0u);
    //                 SPRITE_FLAGS(sprite) = new_flags;

    //                 // Query the surface's current redirection configuration
    //                 GetRedirectionInfo(surface, &redirection_style, &redirection_param1, &redirection_param2, NULL, NULL);

    //                 // Re-read flags (GetRedirectionInfo may update them), then snapshot and clear the sprite's one-shot update token
    //                 uint32_t updated_flags = SPRITE_FLAGS(sprite);
    //                 uint64_t one_shot_token = SPRITE_TOKEN(sprite);
    //                 SPRITE_TOKEN(sprite) = 0;
    //                 int32_t ref_count = SPRITE_REF_COUNT(sprite);

    //                 uint64_t sprite_key = SPRITE_KEY(sprite);
    //                 uint64_t surface_key = SURFACE_KEY(surface);

    //                 // Repack sprite and surface state into the DWM flags word.
    //                 //
    //                 //  Bit layout of dwm_flags:
    //                 //    bit 0    ← updated_flags bit 0
    //                 //    bit 1    ← surface_flags63 bit 0
    //                 //    bits 2–3 ← surface_flags63 bits 2–3
    //                 //    bits 4–6 ← updated_flags bits 1–3  (SPRITE_HIDDEN arrives at bit 6)
    //                 //    bit 7    ← updated_flags bit 6      (SPRITE_HIDDEN_TRANSPARENT)
    //                 uint32_t surface_flags63 = SURFACE_FLAGS63(surface);
    //                 uint32_t dwm_flags = (updated_flags & 1u) | (surface_flags63 & 0xCu) |
    //                                      (2u * ((surface_flags63 & 1u) | (updated_flags & 0x40u) | (4u * (updated_flags & 0xEu))));

    //                 void* dwm_port = UserReferenceDwmApiPort();
    //                 DwmAsyncUpdateSprite(dwm_port, sprite_key, surface_key, dwm_flags, SPRITE_GEOM_DATA(sprite), NULL, redirection_style,
    //                                      redirection_param1, redirection_param2, ref_count >= 1, one_shot_token);
    //             }
    //         }
    //     }

    //     DwmSpriteRef_destroy(&sprite_ref);

    // leave_crit:
    //     DwmCritScope_leave(&crit);

    //     return found_live_sprite;
}