# ASK Cli

## How to build

just run `make && sudo make install`

## How to use

`ask "What is Google"`

## Translation

- `ask tEn "你好，世界"`: Automatically translate between Chinese and English
- `ask tZh "hello world"`: Translate the input into Chinese
- `ask tJp "你好"`: Translate the input into Japanese

## Shell

- `ask shell "list cpp files in the current directory"`: Output a single shell command
- `ask shell "show changed git files" | sh`: Pipe the generated command directly into `sh`
