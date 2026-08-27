# CWE-787 · Out-of-bounds Write — Guion + Preguntas

Presentación de ~15 min. Orden: teoría → demo en vivo → fix en código → preguntas.
Demo: tienda **TechStore** (carpeta `demo-cwe787/`).

---

## Mini-guion cronometrado (~15 min)

### 1) Teoría — ~5 min  (slides 2 a 5)

**Slide 2 — ¿Qué es?**
> "La vulnerabilidad es simple de enunciar: el programa **escribe datos fuera del
> espacio de memoria que reservó**. Reservó un buffer para, digamos, 16 bytes, y termina
> escribiendo en el byte 17, 18, 20… pisando lo que haya al lado."

**Slide 3 — ¿Cómo funciona? (el porqué)**
> "¿Por qué es posible? Por tres cosas que se juntan: (1) los buffers tienen un tamaño
> **fijo**; (2) en memoria las variables viven **pegadas** una al lado de la otra; y (3) en
> lenguajes como C/C++ **nadie verifica el límite** al copiar. Si copiás más datos de los que
> entran, la escritura se **desborda** y sigue pisando la memoria vecina.
> ¿Qué hay en esa memoria vecina? Puede ser otra variable —un precio, un flag de 'es admin'—,
> o la **dirección de retorno** de la función, que es lo que convierte esto en ejecución de código."

**Slide 4 — Casos reales**
> "Esto no es teórico. **FORCEDENTRY (2021)**: el spyware Pegasus entraba a un iPhone sin que
> la víctima tocara nada, explotando un out-of-bounds write en el decodificador de imágenes
> JBIG2. **Stagefright (2015)**: un MMS malicioso comprometía Android parseando el video;
> ~950 millones de dispositivos. Y el patrón clásico —un input largo que pisa un flag— es lo
> que van a ver ahora en vivo. Por algo MITRE lo tiene como el CWE **más peligroso** del ranking."

**Slide 5 — Tabla teórica** (opcional, leer por arriba)
> "Acá está el resumen formal: cómo se introduce (strcpy, memcpy sin control), consecuencias
> (de un crash hasta RCE) y en qué plataformas (C, C++, firmware, navegadores)."

### 2) Demo en vivo — ~4 min  (slide 6 → navegador)

> Levantar antes: `cd demo-cwe787 && docker build -t cwe787-demo . && docker run --rm -p 8000:8000 cwe787-demo` → `http://localhost:8000`.

1. **Cupón inválido** (`DESCUENTO`) → se cobra **$1.200**. "Comportamiento normal."
2. **Cupón válido** (`SALE25`) → **$900**. "25% real, todo bien."
3. **Cupón malicioso** (botón 💥, 20 letras "A") → **Total: GRATIS**.
   > "El cupón no es válido —fíjense `PASSWORD/COUPON_OK = 0`— pero pagué $0. Miren la memoria:
   > los 20 bytes 'A' no entran en el buffer de 16, se desbordan, y los 4 bytes de sobra **pisan
   > `discount_percent`**, que ahora vale mil millones. Escribí fuera de límite y me llevé la notebook gratis."
4. (Slide 7 de apoyo grafica exactamente este paso, por si conviene mostrarlo.)

### 3) El fix — ~3 min  (slide 8, y opcional abrir `vuln.c` / `secure.c`)

> "El bug es una línea: `strcpy(coupon, entrada)` copia **sin límite**. El arreglo es acotar la
> copia al tamaño del buffer: `strncpy(coupon, entrada, sizeof(coupon)-1)` y cerrar con el `\0`.
> Ahora es **imposible** escribir más allá de `coupon[15]`, así que el descuento no se puede tocar."
> En la web: cambiar a **Backend CORREGIDO** y repetir el cupón malicioso → **rechazado, $1.200**.

### 4) Mitigaciones + Preguntas — ~3 min  (slides 9 y 10)

> "Más allá de este caso: usar funciones acotadas, validar siempre longitudes e índices, y
> cuando se pueda, lenguajes memory-safe como Rust. Además defensas del sistema (canaries, ASLR)
> y detección automática (AddressSanitizer, fuzzing). ¿Preguntas?"

---

## Preguntas probables (con respuesta)

**1. ¿Qué diferencia hay con "Buffer Overflow" (CWE-120)?**
Out-of-bounds Write (CWE-787) es el concepto general: **escribir fuera de los límites**, por el
motivo que sea (índice mal, puntero mal, tamaño mal). El "Classic Buffer Overflow" (CWE-120) es un
**caso particular**: copiar a un buffer sin chequear el tamaño de la entrada. Todo CWE-120 termina
en un CWE-787, pero CWE-787 es más amplio.

**2. ¿Y con Out-of-bounds *Read* (CWE-125)?**
El *read* **lee** memoria que no le corresponde (fuga de datos, ej. Heartbleed). El *write*
**escribe** fuera de límite: además de corromper, permite **cambiar el comportamiento** del
programa. El write es generalmente más grave porque puede llevar a ejecución de código.

**3. ¿Por qué C/C++ permiten esto y otros lenguajes no?**
C/C++ no hacen *bounds checking*: un array es un puntero y podés escribir en cualquier offset; el
lenguaje confía en el programador a cambio de velocidad y control. Java, Python, Go, Rust validan
los accesos (o el modelo de ownership los previene) y lanzan una excepción / no compilan en vez de
pisar memoria.

**4. En un sistema real, ¿este overflow no lo frena el sistema operativo?**
Sí, hay mitigaciones: **stack canaries** (un valor centinela antes del return address; si se pisa,
el programa aborta), **ASLR** (aleatoriza direcciones), **DEP/NX** (marca la pila como no
ejecutable), **FORTIFY_SOURCE** (reemplaza strcpy por una versión que conoce el tamaño). En la demo
las **desactivé a propósito** para mostrar el bug crudo. Pero son **defensa en profundidad, no el
arreglo**: el bug sigue ahí y muchos exploits reales (Pegasus, Stagefright) las sortearon. Hay que
arreglar el código.

**5. Si desactivaste las protecciones, ¿la demo no es "trucada"?**
No: la línea vulnerable es real y el desborde ocurre de verdad. Lo único que hago es apagar las
alarmas del compilador para que se **vea** el efecto en vez de que el proceso aborte. Con las
protecciones activas, el mismo `strcpy` seguiría siendo un bug —solo que en lugar de "gratis"
daría un *crash* (DoS), que también es un problema.

**6. ¿Esto puede llegar a ejecución de código (RCE), o solo cambiar una variable?**
En la demo solo piso una variable (el descuento). Pero si lo que está al lado del buffer es la
**dirección de retorno** de la función, sobre-escribiéndola redirigís la ejecución a código del
atacante (shellcode o ROP). Ese es el salto de "dato corrupto" a **RCE**, y es lo que lo hace tan
peligroso.

**7. ¿Stack o heap? ¿Cambia algo?**
El ejemplo es en **stack** (variables locales contiguas). También ocurre en el **heap**: ahí se
pisan datos de otras estructuras o los **metadatos del allocator**, lo que habilita técnicas de
*heap exploitation*. El principio —escribir fuera de lo reservado— es el mismo.

**8. ¿`strncpy` es la solución definitiva?**
Es una mejora clave (acota la copia), pero tiene una trampa: si la entrada llena el buffer,
`strncpy` **no** agrega el `\0`. Por eso en el fix lo agrego a mano (`coupon[15] = 0`). Alternativas
más seguras: `snprintf`, `strlcpy`, o directamente `std::string`/contenedores en C++.

**9. ¿Cómo se detecta esto en la práctica?**
**Análisis estático** (marca strcpy/gets/sprintf y accesos sin validar), **AddressSanitizer**
(instrumenta el binario y aborta con detalle al primer acceso fuera de límite) y **fuzzing**
(bombardea con entradas raras hasta encontrar el crash). Idealmente, todo en el CI.

**10. ¿Un `if (len < 16)` antes de copiar no alcanzaba?**
Validar la longitud es correcto y es parte de la solución. El punto es que hay que hacerlo
**siempre** y de forma consistente; es fácil olvidarlo o equivocarse con un off-by-one. Por eso se
prefiere apoyarse en funciones acotadas y, mejor aún, en lenguajes que lo garantizan por diseño.

**11. ¿Por qué la página web y no algo en C directo?**
Out-of-bounds Write es un bug de **memoria nativa**: no existe en JavaScript puro. La página es solo
la **interfaz** (el checkout); el desborde real ocurre en el binario en C que corre por detrás. El
input viaja como argumento (no por shell), así que lo único vulnerable es el C —que es justamente lo
que queremos mostrar.

---

## Cómo reproducir la demo (instructivo → entregable III)
```bash
cd demo-cwe787
docker build -t cwe787-demo .
docker run --rm -p 8000:8000 cwe787-demo   # abre http://localhost:8000
```
Cupones válidos: `SALE10` (10%) y `SALE25` (25%). Payload de ataque: 20+ caracteres cualesquiera.
Detalle completo en `demo-cwe787/README.md`.
