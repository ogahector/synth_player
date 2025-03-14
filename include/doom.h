#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t row;
    uint8_t col;
} Pixel;

extern const Pixel gameOver[1106];

extern const size_t numGameOver;

extern const Pixel gunOutline[167];

extern const size_t numGunOutline;

extern const Pixel doomEnemy[726];

extern const size_t numEnemy;

extern const Pixel doomGun[63];

extern const size_t numGun;

extern const Pixel doomLoadScreen[1029];

extern const size_t numLoadOnes;
