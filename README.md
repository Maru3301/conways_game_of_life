# `John Conways game of life`
---

C implementation of the well known  
[John Conways game of life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)
using [SDL2](https://www.libsdl.org)  
Not necessarily well optimized, I made it mainly for practice with SDL.  
In the future I may expand upon this.

---

**Usage:**

**./main** _\<cell count x\> \<cell count y\> \<initializer\> \<initializer args\>_

initializer is one of:
* **glider**
* **acorn**
* **blunt_random**
* **perlin** _\<density\>_
* **more to come**

**glider:**
The common glider figure starting in the top left corner

**acorn:**
A simple structure starting in the center that expands surprisingly much

**blunt_random:**
Every cell is initiallized randomly with random() % 2

**perlin:**
Perlin Noise, influenced (badly) by density, where density is a number between 0 and 255

---

Compile (on Linux) with **make**  
**make run** executes it with some parameters  
**make debug** starts gdb with breakpoint at main and layout  
**make valgrind** runs valgrind with leak-check full
