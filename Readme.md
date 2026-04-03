# BurnSSH
Se ejecuta con: Burnssh ./ <time_max>

## main.c

### `sigchld_handler`
Se activa cuando un proceso hijo termina. Lo saca de la lista de activos y lo manda al historial.

### `sigalrm_handler`
Se activa después de 10 segundos en shutdown. Mata todos los procesos que queden vivos con SIGKILL y sale.

### `sigint_handler`
Maneja Ctrl+C. Mata todos los procesos, guarda el historial y sale.

### `main`
Configura los manejadores de señales, muestra el prompt "burnssh>", lee lo que escribe el usuario y llama al comando correspondiente (launch, status, abort, pause, resume, shutdown).

## functions.c

### `add_to_history_with_data`
Agrega un proceso que ya terminó a una lista (el historial). Usa malloc para crear un nodo nuevo.

### `free_history`
Recorre toda la lista del historial y libera la memoria de cada nodo.

### `print_history`
Muestra todos los procesos que ya terminaron (los del historial).

### `find_free_slot`
Busca un espacio vacío en el arreglo de procesos activos (máximo 10). Devuelve el índice o -1 si está lleno.

### `print_active`
Muestra los procesos que están corriendo en este momento.

### `add_active_process`
Guarda un nuevo proceso en la lista de activos: su PID, nombre, hora de inicio, etc.

### `remove_active_process`
Borra un proceso de la lista de activos (pone su PID en 0).

### `get_seconds`
Calcula cuántos segundos lleva corriendo un proceso. Si está pausado, cuenta hasta cuando se pausó. Si terminó, cuenta el tiempo total que vivió.

### `is_number`
Revisa si un string tiene solo números. Devuelve 1 si es número, 0 si no.

## commands.c

### `cmd_launcher`
Lanza un nuevo proceso. Hace fork(), en el hijo ejecuta el programa con execvp(). Si el programa no existe, avisa error. Si existe, lo agrega a la lista de activos. Si hay tiempo máximo, crea un "watcher" que lo mata si se pasa del tiempo.

### `cmd_status`
Muestra los procesos activos y el historial (llama a print_active y print_history).

### `cmd_abort`
Junta todos los PIDs de procesos activos, crea un proceso que espera N segundos y después los mata a todos.

### `cmd_pause`
Pausa un proceso específico. Le manda SIGSTOP, lo marca como pausado y guarda la hora.

### `cmd_resume`
Reanuda un proceso que estaba pausado. Le manda SIGCONT, lo marca como no pausado y ajusta la hora de inicio para que no cuente el tiempo que estuvo detenido.

### `cmd_shutdown`
Manda SIGINT a todos los procesos para que terminen ordenadamente, espera 10 segundos y si queda algo, los mata con SIGKILL. Después sale del programa.

## Aclaración.
Se agregó la lógica para cuando se ejecuta Ctrl + C, y funciona, pero se imprime dos veces el historial y no supimos resolverlo. El que aparece correctamente es el segundo cuadro de historial.