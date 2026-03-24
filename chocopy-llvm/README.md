# Учебный проект chocopy-llvm

Учебный проект `chocopy-llvm` предлагает эскиз компилятора языка chocopy в `LLVM IR`. Язык `chocopy` является подмножеством (диалектом) языка `python 3.6`. Разработка учебного компилятора будет вестись в инфраструктуре `LLVM`, а сам проект `chocopy-llvm` реализуется как внешний проект `LLVM`. На текущий момент проект поддерживает только `STANDALONE` сборку, то есть сборка `chocopy`-llvm выполняется отдельно от основного проекта llvm.

## Быстрый старт

Для работы над проектом необходимо собрать `LLVM` из исходного кода. Предполагается, что сборка самого учебного проекта, а также `LLVM` будет выполнена при помощи llvm-toolchain-а. Поэтому перед началом работы необходимо установить необходимые зависимости (компилятор `clang`, стандартную библиотеку C++ `libc++`, а также компоновщик `lld`, он же `linker`). В качестве системы сборки рекомендуется использовать `Ninja` (В предлагаемых `CMakePresets`, в параметре генератора сценариев сборки указано использование Ninja в качестве целевой системы сборки). Ниже предлагается команда для установки необходимых зависимостей в ОС семейства Ubuntu:
```Bash
sudo apt install cmake ninja-build clang libc++-dev libc++abi-dev lld ccache
```

## Подготовка рабочего окружения.
После установки требуемых зависимостей следует подготовка рабочего окружения. В качестве корневой директории для разработки предлагается директория `chocopy-workdir`. Эта директория будет содержать исходный код проекта `chocopy-llvm` (поддиректория `chocopy-llvm`), исходный код проекта `llvm` (поддиректория `llvm-project`), директорию сборки `llvm` (поддиректория `llvm-build-[debug|release|relwithdebinfo]`), директорию сборки `chocopy-llvm` (поддиректория `chocopy-llvm-build-[debug|release|relwithdebinfo]`).

```Bash
$ mkdir chocopy-workdir
$ cd chocopy-workdir
$ export CHOCOPY_WORKDIR=$(pwd) # Пусть корень рабочей директории хранится тут
$ git clone git@github.com:compiler-courses/<YOUR-TEAM>.git chocopy-llvm # Клонирование chocopy-llvm
$ git clone git@github.com:compiler-courses/llvm-project.git
```

В процессе работы предлагается использовать CMakePreset-ы. В директории [chocopy-llvm/presets](./presets/) исходного кода `chocopy-llvm` содержатся подготовленные CMakePreset-ы для сборки `llvm` в предположении, что рабочая директория соответствует предложенной структуре.

### Сборка llvm.
```Bash
$ cd llvm-project
$ git switch chpy-main
$ cd llvm
$ cp ../../chocopy-llvm/presets/llvm/CMakeUserPresets.json ./
$ cmake —preset debug # Конфигурирование. После успешного конфигурирования:
$ cmake —build —preset debug # Сборка
$ cd ../../ # После успешной сборки вернитесь в корень рабочей директории.
```

### Сборка chocopy-llvm

```Bash
$ cd chocopy-llvm
$ cmake —preset debug # Конфигурирование проекта
$ cmake —build —preset debug # Сборка после успешного конфигурирования
```

После успешной сборки в директории `${CHOCOPY_WORKDIR}/chocopy-llvm-build-debug/bin` будут лежать исполняемые файлы учебного компилятора, а в директории `${CHOCOPY_WORKDIR}/chocopy-llvm-build-debug/lib` его библиотеки.

Следующим шагом необходимо активировать окружение `chocopy-llvm` изменив соответствующим образом переменные среды окружения `PATH` и `LD_LIBRARY_PATH`. Для удобства в корне `chocopy-llvm` лежит вспомогательный скрипт [env.sh](./env.sh), содержащий необходимые команды для активации. Для активации выполните команду:
```Bash
$ source env.sh
```

Для деактивации:
```Bash
$ chocopy-llvm-exit
```

## Компиляция chocopy-программ.
Компиляция chocopy-программы в `LLVM IR`:
```Bash
$ chocopy-llvm -emit-llvm -o demo.ll test/demo/demo.py
```

Связывание скомпилированной программы со стандартной библиотекой `chocopy`:
```Bash
$ clang++ -fuse-ld=lld -Xlinker -T libstdchpy/linker.ld libstdchpy/libstdchpy.cpp libstdchpy/libstdchpy.ll demo.ll -o demo
```

Выполнение скомпилированного артефакта:
```Bash
$ ./demo
220
123
1512975
```
