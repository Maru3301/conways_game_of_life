#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <string.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>

#define WINDOW_TITLE "Once again I am thinking of an amazing title"
#define GRID_TO_WINDOW_SCALE 10
#define FPS 500
#define MSPT 1000 / FPS
#define FRAMESKIPS 100
#define SKIPS_INCREMENT 10
#define MUTATION_THREASHOLD 3

#define RUNNING 0x00
#define PAUSE 0x11
#define QUIT_GAME 0xFF


typedef struct {
    float x, y;
} vector2;

typedef struct{
    uint64_t grid_size[3];

    uint8_t **grid;
    uint8_t **swp_grid;
} convey_enviroment;

typedef struct{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Surface *surface;
    SDL_Event e;

    SDL_Rect *rects;

    uint8_t flag;

    uint32_t frameskips;

    clock_t frame_start_time, frame_finish_time;
    double time_delta;
} sdl_enviroment;

uint32_t renderer(sdl_enviroment *sdl_env, convey_enviroment *con_env);
uint32_t event_handler(SDL_Event e, uint8_t *flag, uint32_t *frameskips);

uint32_t convey_init(convey_enviroment *con_env);
uint32_t convey_advance_generation(convey_enviroment *con_env);

uint32_t Init(sdl_enviroment *sdl_env, convey_enviroment *con_env, char **argv);
uint32_t Exit(sdl_enviroment *sdl_env, convey_enviroment *con_env);

// Utilities
uint32_t str_to_int(char *string, uint64_t *number);
uint32_t coords_rel_to_abs(convey_enviroment *con_env, int64_t *x, int64_t *y, int64_t x_off, int64_t y_off);
float interpolate(float a0, float a1, float weight);
vector2 randomGradient(int ix, int iy);
float dotGridGradient(int ix, int iy, float x, float y);
float perlin(float x, float y);

// Various Init functions for different start scenarios
uint32_t blunt_random(convey_enviroment *con_env);
uint32_t glider_start(convey_enviroment *con_env);
uint32_t acorn_start(convey_enviroment *con_env);
uint32_t testing_start(convey_enviroment *con_env);
uint32_t perlin_start(convey_enviroment *con_env, float density);

// Various Rule functions
uint32_t ruleset_classic(convey_enviroment *con_env);
uint32_t ruleset_testing(convey_enviroment *con_env);

/*
Main
*/

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <Cells x> <Cells y>\n", argv[0]);
        return 1;
    }

    sdl_enviroment *sdl_env = (sdl_enviroment *) malloc(sizeof(sdl_enviroment));
    convey_enviroment *con_env = (convey_enviroment *) malloc(sizeof(convey_enviroment));

    if (Init(sdl_env, con_env, argv))
        return 2;

    printf("Grid of size %ld by %ld\n", con_env->grid_size[0], con_env->grid_size[1]);

    sdl_env->flag = RUNNING;
    uint64_t frame = 0;

    while (sdl_env->flag != QUIT_GAME)
    {
        sdl_env->frame_start_time = clock();
        event_handler(sdl_env->e, &sdl_env->flag, &sdl_env->frameskips);

        if (frame % sdl_env->frameskips == 0)
        {
            if (sdl_env->flag != PAUSE)
            {
                renderer(sdl_env, con_env);
                convey_advance_generation(con_env);
            }
            else
            {
                printf("\r--------PAUSED--------");
            }
        }

        // Ensure stable framerate

        sdl_env->frame_finish_time = clock();
        sdl_env->time_delta = ((double) (sdl_env->frame_finish_time - sdl_env->frame_start_time)) / CLOCKS_PER_SEC;
        if (sdl_env->time_delta < MSPT)
        {
            SDL_Delay(MSPT - sdl_env->time_delta);
            // printf("\rSleeping for %lf ms", MSPT - sdl_env->time_delta);
        }
        else if (sdl_env->time_delta < 100 * MSPT)
        {
            fprintf(stderr, "Time taken too long: %f\n", sdl_env->time_delta);
            return -1;
        }

        frame++;
    }

    Exit(sdl_env, con_env);

    return 0;
}

/*
Convey Logic
*/

uint32_t convey_advance_generation(convey_enviroment *con_env)
{
    // Having random mutations sounds like fun!
    // Then initialize all to 0 and wait for life to form
    // Need to adjust the rules tho, else it would die

    ruleset_classic(con_env);
    // ruleset_testing(con_env);

    // Swap the arrays in the end
    uint8_t **temp = con_env->swp_grid;
    con_env->swp_grid = con_env->grid;
    con_env->grid = temp;
    return 0;
}

// Various Rule functions
uint32_t ruleset_classic(convey_enviroment *con_env)
{
    int32_t sum;
    int64_t x_index;
    int64_t y_index;

    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            sum = 0;

            for (int64_t i = -1; i <= 1; i++)
            {
                for (int64_t j = -1; j <= 1; j++)
                {
                    x_index = x;
                    y_index = y;
                    coords_rel_to_abs(con_env, &x_index, &y_index, i, j);

                    sum += con_env->grid[x_index][y_index];
                }
            }

            sum -= con_env->grid[x][y];

            if (sum < 2 | sum > 3)
            {
                con_env->swp_grid[x][y] = 0;
            }
            else if (sum == 3)
            {
                con_env->swp_grid[x][y] = 1;
            }
            else
            {
                con_env->swp_grid[x][y] = con_env->grid[x][y];
            }
        }
    }

    return 0;
}

uint32_t ruleset_testing(convey_enviroment *con_env)
{
    int32_t sum;
    int64_t x_index;
    int64_t y_index;

    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            sum = 0;

            for (int64_t i = -1; i <= 1; i++)
            {
                for (int64_t j = -1; j <= 1; j++)
                {
                    x_index = x;
                    y_index = y;
                    coords_rel_to_abs(con_env, &x_index, &y_index, i, j);

                    sum += con_env->grid[x_index][y_index];
                }
            }

            sum -= con_env->grid[x][y];

            if (sum < 2 | sum > 3)
            {
                con_env->swp_grid[x][y] = 0;
            }
            else if (sum == 3)
            {
                con_env->swp_grid[x][y] = 1;
            }
            else
            {
                con_env->swp_grid[x][y] = con_env->grid[x][y];
            }
        }
    }

    return 0;
}

uint32_t convey_init(convey_enviroment *con_env)
{
    // maybe later perlin noise?

    // Make sure all is zero = dead
    // Could also use memset but meh, it only runs once, it's fine

    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            con_env->grid[x][y] = 0;
        }
    }

    perlin_start(con_env, 0.9);
    // blunt_random(con_env);
    // glider_start(con_env);
    // acorn_start(con_env);
    // testing_start(con_env);

    // Copy once to swp_grid so we're on the same page
    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            con_env->swp_grid[x][y] = con_env->grid[x][y];
        }
    }

    return 0;
}

// Various Init functions starting here
uint32_t blunt_random(convey_enviroment *con_env)
{
    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            con_env->grid[x][y] = random() % 2;
        }
    }

    return 0;
}

uint32_t perlin_start(convey_enviroment *con_env, float density)
{
    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            con_env->grid[x][y] = round((density + perlin((float) x + 0.5, (float) y + 0.5)) / 2);
        }
    }

    return 0;
}

uint32_t glider_start(convey_enviroment *con_env)
{
    // Add a glider
    con_env->grid[1][0] = 1;
    con_env->grid[2][1] = 1;
    con_env->grid[0][2] = 1;
    con_env->grid[1][2] = 1;
    con_env->grid[2][2] = 1;

    return 0;
}

uint32_t acorn_start(convey_enviroment *con_env)
{
    uint64_t x_center = con_env->grid_size[0] / 2;
    uint64_t y_center = con_env->grid_size[1] / 2;

    con_env->grid[x_center][y_center] = 1;
    con_env->grid[x_center + 1][y_center - 1] = 1;
    con_env->grid[x_center + 2][y_center - 1] = 1;
    con_env->grid[x_center + 3][y_center - 1] = 1;
    con_env->grid[x_center - 2][y_center - 1] = 1;
    con_env->grid[x_center - 3][y_center - 1] = 1;
    con_env->grid[x_center - 2][y_center + 1] = 1;

    return 0;
}

uint32_t testing_start(convey_enviroment *con_env)
{
    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            con_env->grid[x][y] = (x + y) % 2;
        }
    }

    return 0;
}

/*
SDL Logic
*/

uint32_t renderer(sdl_enviroment *sdl_env, convey_enviroment *con_env)
{
    for (uint32_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint32_t y = 0; y < con_env->grid_size[1]; y++)
        {
            if (con_env->grid[x][y])
            {
                // sdl_env->rects[x * con_env->grid_size[1] + y].x = GRID_TO_WINDOW_SCALE * x;
                // sdl_env->rects[x * con_env->grid_size[1] + y].y = GRID_TO_WINDOW_SCALE * y;
                sdl_env->rects[x * con_env->grid_size[1] + y].w = GRID_TO_WINDOW_SCALE;
                sdl_env->rects[x * con_env->grid_size[1] + y].h = GRID_TO_WINDOW_SCALE;
            }
            else
            {
                // sdl_env->rects[x * con_env->grid_size[1] + y].x = 0;
                // sdl_env->rects[x * con_env->grid_size[1] + y].y = 0;
                sdl_env->rects[x * con_env->grid_size[1] + y].w = 0;
                sdl_env->rects[x * con_env->grid_size[1] + y].h = 0;
            }
        }
    }

    // Full Screen to 10 10 10 = "Black"
    SDL_SetRenderDrawColor(sdl_env->renderer, 10, 10, 10, 255);
    SDL_RenderClear(sdl_env->renderer);

    // Fill rectangles 10 100 10 = green
    SDL_SetRenderDrawColor(sdl_env->renderer, 10, 100, 10, 255);
    SDL_RenderFillRects(sdl_env->renderer,
                        sdl_env->rects,
                        con_env->grid_size[2]);

    // Outline rectangles 30 10 100 = purple
    SDL_SetRenderDrawColor(sdl_env->renderer, 30, 10, 100, 255);
    SDL_RenderDrawRects(sdl_env->renderer,
                        sdl_env->rects,
                        con_env->grid_size[2]);

    SDL_RenderPresent(sdl_env->renderer);

    return 0;
}

uint32_t event_handler(SDL_Event e, uint8_t *flag, uint32_t *frameskips)
{
    if (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            *flag = QUIT_GAME;
        }
        else if (e.type == SDL_KEYDOWN)
        {
            // printf("Pressed: %d = %s\n", e.key.keysym.scancode, SDL_GetKeyName(key));

            switch (e.key.keysym.sym)
            {
                case SDLK_ESCAPE:
                    if (*flag != PAUSE)
                        *flag = PAUSE;
                    else
                        *flag = RUNNING;
                    break;

                case SDLK_q:
                    *flag = QUIT_GAME;
                    break;

                // Equiv. to the inverse of the simulation speed
                // Don't you agree linear is a bad choice?
                // Change that later maybe. Or don't.

                case SDLK_PLUS:
                    if (*frameskips > 10)
                        *frameskips -= SKIPS_INCREMENT;
                    break;

                case SDLK_MINUS:
                    *frameskips += SKIPS_INCREMENT;
                    break;

                default:
                    printf("Pressed Key does not have an effect: %s\n", SDL_GetKeyName(e.key.keysym.sym));
            }
        }
    }

    return 0;
}

/*
Init
*/

uint32_t Init(sdl_enviroment *sdl_env, convey_enviroment *con_env, char **argv)
{
    // Get everything prepared

    if (str_to_int(argv[1], &con_env->grid_size[0]))
    {
        return 1;
    }
    if (str_to_int(argv[2], &con_env->grid_size[1]))
    {
        return 1;
    }

    con_env->grid_size[2] = con_env->grid_size[0] * con_env->grid_size[1];

    // Startup SDL

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 2;
    }
    atexit(SDL_Quit);

    sdl_env->window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        GRID_TO_WINDOW_SCALE * con_env->grid_size[0],
        GRID_TO_WINDOW_SCALE * con_env->grid_size[1],
        SDL_WINDOW_OPENGL //| SDL_WINDOW_MAXIMIZED
    );
    if (!sdl_env->window)
    {
        fprintf(stderr, "Error creating window:\n%s\n", SDL_GetError());
        return 3;
    }

    sdl_env->renderer = SDL_CreateRenderer(
        sdl_env->window,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    if (!sdl_env->renderer)
    {
        fprintf(stderr, "Error creating renderer:\n%s\n", SDL_GetError());
        return 4;
    }

    sdl_env->surface = SDL_CreateRGBSurface(0,
        con_env->grid_size[0] * GRID_TO_WINDOW_SCALE,
        con_env->grid_size[1] * GRID_TO_WINDOW_SCALE,
        32, 0, 0, 0, 0);
    if (!sdl_env->surface)
    {
        fprintf(stderr, "Error creating surface:\n%s\n", SDL_GetError());
        return 5;
    }

    sdl_env->rects = (SDL_Rect *) malloc(sizeof(SDL_Rect) * con_env->grid_size[2]);
    if (!sdl_env->rects)
    {
        fprintf(stderr, "Memory allocation failed for sdl_env->rects\n");
        return 6;
    }
    for (uint32_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint32_t y = 0; y < con_env->grid_size[1]; y++)
        {
            sdl_env->rects[x * con_env->grid_size[1] + y].x = GRID_TO_WINDOW_SCALE * x;
            sdl_env->rects[x * con_env->grid_size[1] + y].y = GRID_TO_WINDOW_SCALE * y;
        }
    }

    sdl_env->frameskips = FRAMESKIPS;

    // Prepare Convey
    con_env->grid = (uint8_t **) malloc(con_env->grid_size[2] * sizeof(uint8_t));
    if (!con_env->grid)
    {
        fprintf(stderr, "Memory allocation failed for con_env->grid\n");
        return 7;
    }

    con_env->swp_grid = (uint8_t **) malloc(con_env->grid_size[2] * sizeof(uint8_t));
    if (!con_env->swp_grid)
    {
        fprintf(stderr, "Memory allocation failed for con_env->swp_grid\n");
        return 8;
    }

    for (uint32_t x = 0; x < con_env->grid_size[0]; x++)
    {
        con_env->grid[x] = (uint8_t *) malloc(con_env->grid_size[1] * sizeof(uint8_t));
        con_env->swp_grid[x] = (uint8_t *) malloc(con_env->grid_size[1] * sizeof(uint8_t));
    }

    convey_init(con_env);

    return 0;
}

/*
Exit
*/
uint32_t Exit(sdl_enviroment *sdl_env, convey_enviroment *con_env)
{
    SDL_DestroyRenderer(sdl_env->renderer);
    SDL_DestroyWindow(sdl_env->window);

    free(sdl_env->rects);

    free(con_env->grid);
    free(con_env->swp_grid);

    free(con_env);
    free(sdl_env);

    SDL_Quit();
    return 0;
}

/*
Utilities
str_to_int: convertes a (max 19 digit) string-number into a uint64_t
*/

uint32_t coords_rel_to_abs(convey_enviroment *con_env, int64_t *x, int64_t *y, int64_t x_off, int64_t y_off)
{
    *x = (*x + x_off + con_env->grid_size[0]) % con_env->grid_size[0];
    *y = (*y + y_off + con_env->grid_size[1]) % con_env->grid_size[1];

    return 0;
}

// Create random direction vector
vector2 randomGradient(int ix, int iy) {
    // Random float. No precomputed gradients mean this works for any number of grid coordinates
    float random = 2920.f * sin(ix * 21942.f + iy * 171324.f + 8912.f) * cos(ix * 23157.f * iy * 217832.f + 9758.f);
    return (vector2) { .x = cos(random), .y = sin(random) };
}

// interpolates between a0 and a1, weight e [0.0, 1.0]
float interpolate(float a0, float a1, float weight) {
    return pow(weight, 2.0) * (3.0 - 2.0 * weight) * (a1 - a0) + a0;
}

// Computes the dot product of the distance and gradient vectors.
float dotGridGradient(int ix, int iy, float x, float y) {
    // Get gradient from integer coordinates
    vector2 gradient = randomGradient(ix, iy);

    // Compute the distance vector
    float dx = x - (float)ix;
    float dy = y - (float)iy;

    // Compute the dot-product
    return (dx*gradient.x + dy*gradient.y);
}

// Compute Perlin noise at coordinates x, y
float perlin(float x, float y) {
    // Determine grid cell coordinates
    int x0 = (int)x;
    int x1 = x0 + 1;
    int y0 = (int)y;
    int y1 = y0 + 1;

    // Determine interpolation weights
    // Could also use higher order polynomial/s-curve here
    float sx = x - (float)x0;
    float sy = y - (float)y0;

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y0, x, y);
    ix0 = interpolate(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = interpolate(n0, n1, sx);

    value = interpolate(ix0, ix1, sy);
    return value;
}

uint32_t str_to_int(char *string, uint64_t *number)
{
    *number = 0;
    uint8_t c;
    int len = strlen(string);

    if (strlen(string) > 19)
    {
        fprintf(stderr, "Number too large: %s\n", string);
        return 1;
    }

    for (int i = 0; i < len; i++)
    {
        c = string[i];

        if (c - 0x30 <= 0x9)
        {
            *number += (c - 0x30) * pow(10, len - i - 1);
        }
        else
        {
            fprintf(stderr, "Not a Number: %c; letter %d in %s\n", c, i + 1, string);
            return 2;
        }
    }

    return 0;
}
