#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <string.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>

#define WINDOW_TITLE "Again I am thinking of an amazing title"
#define GRID_TO_WINDOW_SCALE 10
#define FPS 144
#define FRAMESKIPS 144
#define MSPT 1000 / FPS

#define RUNNING 0x00
#define PAUSE 0x11
#define QUIT_GAME 0xFF

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

    clock_t frame_start_time, frame_finish_time;
    double time_delta;
} sdl_enviroment;

uint32_t renderer(sdl_enviroment *sdl_env, convey_enviroment *con_env);
uint32_t event_handler(SDL_Event e, uint8_t *flag);

uint32_t convey_init(convey_enviroment *con_env);
uint32_t convey_advance_generation(convey_enviroment *con_env);

uint32_t Init(sdl_enviroment *sdl_env, convey_enviroment *con_env, char **argv);
uint32_t Exit(sdl_enviroment *sdl_env, convey_enviroment *con_env);

uint32_t str_to_int(char *string, uint64_t *number);

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
        event_handler(sdl_env->e, &sdl_env->flag);

        if (frame % FRAMESKIPS == 0)
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
    uint8_t sum;
    uint64_t x_index;
    uint64_t y_index;

    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            sum = 0;

            for (int8_t i = -1; i <= 1; i++)
            {
                x_index = (x + i) % con_env->grid_size[0];

                for (int8_t j = -1; j <= 1; j++)
                {
                    y_index = (y + j) % con_env->grid_size[1];

                    sum += con_env->grid[x_index][y_index];
                }
            }

            if (sum < 2 | sum > 3)
            {
                con_env->swp_grid[x][y] = 0;
            }
            else
            {
                con_env->swp_grid[x][y] = 1;
            }
        }
    }

    // Swap the arrays in the end
    //printf("%lx & %lx -> ", con_env->grid, con_env->swp_grid);
    uint8_t **temp = con_env->swp_grid;
    con_env->swp_grid = con_env->grid;
    con_env->grid = temp;
    //printf("%lx & %lx\n", con_env->grid, con_env->swp_grid);
    return 0;
}

uint32_t convey_init(convey_enviroment *con_env)
{
    // maybe later perlin noise?

    for (uint64_t x = 0; x < con_env->grid_size[0]; x++)
    {
        for (uint64_t y = 0; y < con_env->grid_size[1]; y++)
        {
            con_env->grid[x][y] = random() % 2;
        }
    }
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
                sdl_env->rects[x * con_env->grid_size[1] + y].x = GRID_TO_WINDOW_SCALE * x;
                sdl_env->rects[x * con_env->grid_size[1] + y].y = GRID_TO_WINDOW_SCALE * y;
                sdl_env->rects[x * con_env->grid_size[1] + y].w = GRID_TO_WINDOW_SCALE;
                sdl_env->rects[x * con_env->grid_size[1] + y].h = GRID_TO_WINDOW_SCALE;
            }
            else
            {
                sdl_env->rects[x * con_env->grid_size[1] + y].x = 0;
                sdl_env->rects[x * con_env->grid_size[1] + y].y = 0;
                sdl_env->rects[x * con_env->grid_size[1] + y].w = 0;
                sdl_env->rects[x * con_env->grid_size[1] + y].h = 0;
            }
        }
    }

    SDL_SetRenderDrawColor(sdl_env->renderer, 10, 10, 10, 255);
    SDL_RenderClear(sdl_env->renderer);

    SDL_SetRenderDrawColor(sdl_env->renderer, 10, 100, 10, 255);
    SDL_RenderFillRects(sdl_env->renderer,
                        sdl_env->rects,
                        con_env->grid_size[2]);

    SDL_SetRenderDrawColor(sdl_env->renderer, 10, 10, 100, 255);
    SDL_RenderDrawRects(sdl_env->renderer,
                        sdl_env->rects,
                        con_env->grid_size[2]);

    SDL_RenderPresent(sdl_env->renderer);

    return 0;
}

uint32_t event_handler(SDL_Event e, uint8_t *flag)
{
    SDL_Keycode key;

    if (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            *flag = QUIT_GAME;
        }
        else if (e.type == SDL_KEYDOWN)
        {
            key = e.key.keysym.sym;

            printf("Pressed: %d = %s\n", e.key.keysym.scancode, SDL_GetKeyName(key));

            if (key == SDLK_ESCAPE)
            {
                if (*flag != PAUSE)
                    *flag = PAUSE;
                else
                    *flag = RUNNING;
            }
            else if (key == SDLK_q)
            {
                *flag = QUIT_GAME;
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
        SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED
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

    // Prepare Convey
    con_env->grid = (uint8_t **) malloc(con_env->grid_size[0] * sizeof(uint8_t *));
    if (!con_env->grid)
    {
        fprintf(stderr, "Memory allocation failed for con_env->grid\n");
        return 7;
    }

    con_env->swp_grid = (uint8_t **) malloc(con_env->grid_size[0] * sizeof(uint8_t *));
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
