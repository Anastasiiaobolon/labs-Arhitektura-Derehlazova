# Lab 3. Побудова програми, ELF та організація пам'яті

**Середовище:** GitHub Codespaces, Ubuntu, `gcc`, `file`, `readelf`, `objdump`, `nm`, `ldd`, `size`.
Проміжні файли (`*.i`, `*.s`, `*.o`) та executable в репозиторій не додаються: їх можна відтворити з вихідного коду.

## Файли

`program.c`, `main.c`, `calculate.c`, `sections.c`, `process.c`, `memory.c`, `stack.c`, `heap.c`, `memory_map.c`

## 1. Build pipeline

```bash
gcc -Wall -Wextra program.c -o program && ./program     # Result: 30
gcc -E program.c -o program.i                            # preprocessing
gcc -S -O0 -masm=intel program.i -o program.s            # compilation
gcc -c program.s -o program.o                            # assembling
gcc program.o -o program                                 # linking
```

| Файл | Етап | Що містить | Чи може CPU виконати безпосередньо? |
|---|---|---|---|
| `program.c` | початковий код | C source | ні |
| `program.i` | після preprocessor | C-код з розгорнутими `#include` та макросами | ні |
| `program.s` | після compiler | текст Assembly | ні |
| `program.o` | після assembler | relocatable object file: machine code + таблиця symbols | ні (не злінковано) |
| `program` | після linker | ELF executable | так (після завантаження в пам'ять ОС) |

**Preprocessing.** `wc -l program.c program.i`: `program.i` значно більший, бо `#include <stdio.h>` замінено на повний вміст заголовкових файлів. `16 рядків у program.c та 829 рядків у program.i`. `program.i` це все ще C-код, не machine code, і CPU не може його виконати.

**Compilation.** `program.s` це Assembly (текст) для x86-64 з мітками `add:` та `main:`; це не machine code.

**Assembling.** `program.o` бінарний (`file program.o` показує *ELF 64-bit LSB relocatable*), тому `cat` показує нечитабельні символи. `objdump -d -M intel program.o` знову показує `<add>` та `<main>`, але вони дизасемблюються з байтів.

**Symbols (`nm program.o`).** `main` та `add` визначені в object file (тип `T`), `global_value` у `.data` (`D`), `global_empty` у `.bss` (`B`), `printf` це `U` (undefined, реалізація поза файлом). `program.o` містить реалізацію `add()` та `main()`, але **не** `printf()`. Адресу `printf` підставляє linker, підключаючи бібліотеку.

## 2. Linker error та виправлення

```bash
gcc -c main.c -o main.o          # успішно
gcc main.o -o app                # помилка
```
```text
undefined reference to `calculate'
```
**Чому compilation пройшла, а linking ні?** Для компіляції `main.c` достатньо оголошення `int calculate(int, int);`, тому compiler знає, як викликати функцію. Але самого коду `calculate` немає в `main.o`. Linker не знаходить визначення symbol і видає помилку.

Виправлення: створено `calculate.c`, `gcc -c calculate.c -o calculate.o`, потім `gcc main.o calculate.o -o app && ./app` виводить `30`. `nm main.o` показує `calculate` як `U` (потрібен), `nm calculate.o` показує його як `T` (визначений).

## 3. ELF

```bash
file program
readelf -h program
```

| ELF property | Значення |
|---|---|
| Class | ELF64 |
| Data | 2's complement, little endian |
| Type | DYN (Position-Independent Executable) |
| Machine | Advanced Micro Devices X86-64 |
| Entry point | `0x1060` |

**ELF64 та Machine:** формат файлу 64-бітний; Machine показує архітектуру процесора, для якого створений код (x86-64).

## 4. ELF sections (`readelf -S program`)

| Section | Що містить | У нашій програмі |
|---|---|---|
| `.text` | machine code | функції `main`, `add` |
| `.rodata` | константи, лише читання | рядок `"Result: %d\n"`, у `sections.c` ще `message[]` (`const`) |
| `.data` | ініціалізовані глобальні/static змінні | `int global_value = 10;` |
| `.bss` | неініціалізовані глобальні/static (нулі) | `int global_empty;` |

### `sections.c`: де яка змінна

| Змінна | Секція |
|---|---|
| `initialized = 123` | `.data` |
| `uninitialized` | `.bss` |
| `static_initialized = 456` | `.data` |
| `static_uninitialized` | `.bss` |
| `message[]` (const) | `.rodata` |

Перевірка: `nm sections | grep -E "initialized|message"` та `readelf -S sections`.

## 5. Розмір областей (`size`) та велика `.bss`

```bash
gcc sections.c -o sections && size sections && ls -lh sections
```
```text
ls -l sections
   text    data     bss     dec     hex filename
   1450     608      16    2074     81a sections
-rwxrwxrwx 1 codespace codespace 16144 Nov 22 01:16 sections
```

Далі у тимчасовій копії /tmp/sections_big.c замінено `int uninitialized;` на `int large_array[100000];` (і в `printf` використано `large_array[0]` замість `uninitialized`):
```text
text    data     bss     dec     hex filename
   1450     608  400040  402098   622b2 /tmp/sections_big
-rwxr-xrwx 1 codespace codespace 16160 Nov 22 01:21 /tmp/sections_big
```

**Результат:** `bss` збільшився приблизно на 400 000 байтів (100000 × 4), `text` та `data` майже не змінились, розмір самого файлу практично той самий.

**Чому так?** `.bss` містить неініціалізовані дані, які мають бути нулями. У файлі зберігається лише **інформація про розмір** секції, самих нулів у файлі немає. Операційна система виділить і обнулить пам'ять під час завантаження процесу. Тому велика `.bss` збільшує пам'ять процесу, а не розмір executable.

## 6. Dynamic libraries (`ldd program`)

```text
linux-vdso.so.1 (0x00007ffd33ffd000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007de544bc2000)
        /lib64/ld-linux-x86-64.so.2 (0x00007de544de3000)
```
- **Чи є повна `printf()` у нашому executable?** Ні, реалізація знаходиться в `libc.so.6`, executable лише посилається на неї.
- **Що означає `.so`?** Shared object, динамічна (спільна) бібліотека.
- **Чому кілька програм можуть використовувати одну shared library?** ОС завантажує код бібліотеки в пам'ять один раз і відображає його в адресні простори різних процесів.
- **Перевага:** економія пам'яті й місця на диску порівняно з копіюванням коду бібліотеки в кожен executable, а також бібліотеку можна оновити окремо.

## 7. Executable та process (`process.c`)

```bash
gcc -Wall -Wextra process.c -o process && ./process        # термінал 1
ps -p <PID>                                                 # термінал 2
```
```text
PID: 28261  ps -p 28261
```
`process` це файл на диску (інструкції та дані). Запущений `./process` це процес із власним PID, адресним простором (code, data, heap, stack), станом виконання та ресурсами. З одного executable можна запустити кілька процесів.

## 8. Де знаходяться дані (`memory.c`)

```bash
gcc -Wall -Wextra -O0 memory.c -o memory && ./memory
```
```text
Function main:         0x5e1dd56561a9
Global initialized:    0x5e1dd5659010
Global uninitialized:  0x5e1dd565901c
Static initialized:    0x5e1dd5659014
Static uninitialized:  0x5e1dd5659020
Local variable:        0x7fff4a02b38c
Dynamic allocation:    0x5e1de0f4f2a0
```

| Об'єкт | Очікувана область | Фактично |
|---|---|---|
| main | code (.text) | code |
| global_initialized | data | data |
| global_uninitialized | bss | bss |
| static_initialized | data | data |
| static_uninitialized | bss | bss |
| local | stack | stack |
| dynamic allocation | heap | heap |

(У «Прогнозі» очікування таке саме; у «Фактично» звірте з вашими адресами.)

**Інтерпретація адрес:**
- Поруч знаходяться адреси `main` (code) і, трохи далі, адреси global/static змінних (адреси відрізняються на кілька байтів, змінні `.data` та `.bss` лежать поруч).
- `global_initialized` і `global_uninitialized` лежать близько, але в різних секціях (`.data` / `.bss`).
- `local` (адреса починається з `0x7ff...`) знаходиться **далеко** від глобальних змінних, у ділянці стека.
- Пам'ять від `malloc` (heap) знаходиться окремо від stack: heap ближче до даних програми, stack у верхній частині адресного простору.
- Висновок: адресний простір розбитий на області (code, data/bss, heap, stack).

**Чи однакові адреси при кожному запуску?** Запустіть `./memory` тричі й вставте результат:
```text
    Run 1: main = 0x5aeef184d1a9, local = 0x7ffe0b50bf1c, dynamic = 0x5aef14f652a0.
    Run 2: main = 0x613e8e0141a9, local = 0x7ffc8ed9fe3c, dynamic = 0x613ea91f62a0.
    Run 3: main = 0x5e471464d1a9, local = 0x7ffd687ef3dc, dynamic = 0x5e47532de2a0.
```
Адреси змінюються між запусками. Це не помилка, це **ASLR** (Address Space Layout Randomization): ОС навмисно випадково розташовує області процесу (код PIE, heap, stack) в адресному просторі для безпеки.

## 9. Stack (`stack.c`)

```bash
gcc -Wall -Wextra -O0 stack.c -o stack && ./stack
```
```text
gcc -Wall -Wextra -O0 heap.c -o heap
```
(У типовому результаті `&a` > `&b` > `&c`, наприклад `...28d4`, `...28b4`, `...2894`.)

- **Чи змінюються адреси при вкладених викликах?** Так, кожен виклик створює свій stack frame.
- **У якому напрямку?** У нашому експерименті адреси **зменшуються** (кожен новий виклик глибше → менша адреса).
- **Чи узгоджується з x86-64?** Так, на x86-64 стек зростає у бік **менших адрес**. Це висновок для нашого середовища, а не універсальне правило для всіх ISA.

## 10. Heap (`heap.c`)

```text
level1: &a = 0x7fff64644c04
level2: &b = 0x7fff64644be4
level3: &c = 0x7fff64644bc4
a: 0x5fe9c9d6c2a0
b: 0x5fe9c9d6c2c0
c: 0x5fe9c9d6c2e0
```
Адреси `a`, `b`, `c` близькі між собою й зазвичай зростають (наприклад, з кроком 0x20), бо `malloc` виділяє блоки в області heap, і кожен блок має службові дані allocator.

**Різниця stack та heap:** stack керується автоматично (локальні змінні, виклики функцій, час життя до виходу з функції), heap виділяється вручну (`malloc`/`free`), живе, поки програміст не звільнить пам'ять.

## 11. Фінальне завдання. `memory_map.c`

```bash
gcc -E memory_map.c -o memory_map.i
gcc -S -O0 -masm=intel memory_map.i -o memory_map.s
gcc -c memory_map.s -o memory_map.o
gcc memory_map.o -o memory_map
./memory_map
```
```text
Global initialized:      0x62a17b8f9010
Global uninitialized:    0x62a17b8f9024
Static initialized:      0x62a17b8f9014
Static uninitialized:    0x62a17b8f9028
String literal:          0x62a17b8f7004
Heap allocation:         0x62a180f1e2a0
Stack/local:             0x7ffdfda2d5fc
```

Концептуальна карта пам'яті цієї програми:

```text
Higher addresses
┌──────────────────────┐
│        Stack         │  local
├──────────────────────┤
│          ...         │
├──────────────────────┤
│        Heap          │  malloc allocation (heap_value)
├──────────────────────┤
│        .bss          │  global_uninitialized, static_uninitialized
├──────────────────────┤
│        .data         │  global_initialized, static_initialized, string_literal (pointer)
├──────────────────────┤
│       .rodata        │  "Computer Systems" (string literal)
├──────────────────────┤
│        .text         │  main, show_function
└──────────────────────┘
Lower addresses
```
(Вказівник `string_literal` лежить у `.data`, а сам рядок, на який він вказує, у `.rodata`. Для PIE-програми heap та stack розташовані за рандомізованими адресами.)

## Відповіді на контрольні питання

- **Етапи до executable:** preprocessing → compilation → assembling → linking.
- **Preprocessor:** обробляє директиви (`#include`, `#define`), розгортає макроси, прибирає коментарі.
- **Compiler:** перетворює C на Assembly.
- **Assembler:** перетворює Assembly на object file (machine code).
- **Object file:** machine code, дані, таблицю symbols і relocations.
- **Навіщо linker:** об'єднує object files та бібліотеки, зв'язує посилання на symbols.
- **Undefined reference:** linker не знайшов визначення symbol, який потрібен.
- **Symbol:** ім'я функції чи змінної в object file/executable.
- **Header та library:** header містить оголошення (для compiler), library містить реалізацію (для linker).
- **ELF:** формат виконуваних файлів, object files та бібліотек в Linux.
- **`.text`:** machine code програми.
- **`.rodata`:** константні дані лише для читання (рядкові літерали, `const`).
- **`.data` та `.bss`:** `.data` це ініціалізовані глобальні/static дані (значення зберігаються у файлі), `.bss` це неініціалізовані (у файлі зберігається лише розмір).
- **Чому велика `.bss` не збільшує файл:** у файлі нулі не зберігаються, пам'ять обнулюється при завантаженні.
- **`ldd`:** показує shared libraries, від яких залежить executable.
- **Executable та process:** executable це файл, process це запущена програма з PID і власним адресним простором.
- **Де local variables:** на stack.
- **Для чого heap:** для динамічно виділюваної пам'яті (`malloc`), час життя якої обирає програміст.
- **Stack та heap:** stack автоматичний, LIFO, швидкий, обмежений розміром; heap ручний, гнучкий, потребує `free`.
- **Чому адреси змінюються:** ASLR.
- **Temporal locality:** до тих самих даних, до яких щойно зверталися, ймовірно, звернуться знову незабаром.
- **Spatial locality:** після звернення до адреси ймовірні звернення до сусідніх адрес (наприклад, елементи масиву).

## Висновок

Експериментально пройдено весь шлях `program.c → .i → .s → .o → executable`, відтворено й виправлено linker error, досліджено ELF (ELF64, little-endian, x86-64) і його секції. Підтверджено, що неініціалізовані дані (`.bss`) не збільшують розмір файлу. `ldd` показує динамічне підключення `libc`. Адреси змінних групуються за областями (code, data/bss, heap, stack), а ASLR змінює їх між запусками.
