# 🚀 MAKE IN C: JOGOS DE ESTRATÉGIA — Código-Fonte Oficial

Repositório oficial com os exemplos dos capítulos, o motor ASCII, os quatro jogos de estratégia e o projeto final do livro **MAKE IN C: JOGOS DE ESTRATÉGIA**, o segundo livro da série *Make in C*: um guia prático para aprender **linguagem C do zero absoluto** construindo jogos de estratégia completos, desenhados com caracteres, diretamente no terminal.

```
 _  _  _____   _____    __  __ _   _ _  _ ___   ___
| \| |/ _ \ \ / / _ \  |  \/  | | | | \| |   \ / _ \
| .` | (_) \ V / (_) | | |\/| | |_| | .` | |) | (_) |
|_|\_|\___/ \_/ \___/  |_|  |_|\___/|_|\_|___/ \___/
          simulador de colonização espacial
```

---

## 📖 Sobre o livro

Em **MAKE IN C: JOGOS DE ESTRATÉGIA**, você começa no primeiro `printf` e termina construindo o **Novo Mundo**, um simulador de colonização espacial em tempo real, inspirado no OGame, com quase dezesseis mil linhas de C: um universo gerado a partir do seu nome e do instante em que você começou a jogar, economia que continua com o jogo fechado, frotas, combate, eventos decididos no d20, comércio e muito mais.

### O que você vai dominar na prática

* **C do zero (Parte I):** variáveis, decisões, laços, funções, arrays, recursão, estruturas, ponteiros, memória dinâmica, vários arquivos, `make`, arquivos e bits. Projetos: Palitos, jogo da velha invencível (minimax) e o Reino de Hamurabi.
* **O terminal como tabuleiro (Parte II):** sequências ANSI, cores de 24 bits, UTF-8, modo cru, tela virtual, laço do jogo, interface de estratégia, acaso controlado, mundos procedurais, arte com caracteres, caminhos (BFS, Dijkstra, A\*) e economia.
* **Quatro jogos de estratégia (Parte III):** Batalha Naval, Mercador das Estrelas, Conquista (um pequeno 4X) e Colônia (um jogo incremental em tempo real).
* **Novo Mundo (Parte IV):** um projeto grande, módulo por módulo, com testes automáticos, piloto automático e simulação.
* **Além do livro (Parte V):** desenho de jogos de estratégia e um esqueleto pronto para o seu próprio jogo.

---

## 🛒 Onde adquirir o livro completo

O livro traz a explicação passo a passo, a arquitetura do código e a teoria por trás de cada linha deste repositório:

* 👉 **[Comprar na Amazon (Kindle / e-book)](COLE_AQUI_O_LINK_DA_AMAZON)**
* 👉 **[Comprar na Hotmart (PDF)](COLE_AQUI_O_LINK_DA_HOTMART)** *(em breve)*

---

## 📂 Estrutura do repositório

```text
├── exemplos/    # Os programas dos capítulos 1 a 25, um diretório por capítulo
├── motor/       # O motor da Parte II: term, tela, ui, rng, ruido, caminho
├── batalha/     # Projeto 1: Batalha Naval (capítulo 26)
├── mercador/    # Projeto 2: Mercador das Estrelas (capítulo 27)
├── conquista/   # Projeto 3: Conquista, um mini 4X (capítulo 28)
├── colonia/     # Projeto 4: Colônia, construção em tempo real (capítulo 29)
├── novo-mundo/  # O projeto final: Novo Mundo (capítulos 30 a 44)
└── esqueleto/   # Posto Avançado: o ponto de partida do SEU jogo (capítulo 46)
```

---

## ⚙️ Como compilar e jogar

### Pré-requisitos

* **Um compilador C11:** o GCC no Linux e no macOS (ou o clang), e no Windows o **MSYS2**, o **w64devkit** ou o TDM-GCC. O capítulo 1 mostra a instalação passo a passo. O Novo Mundo também compila com o Visual Studio.
* **Um terminal de verdade:** pelo menos **100 × 30** caracteres para os jogos da Parte III e **120 × 36** para o Novo Mundo. No Windows, use o **Windows Terminal** (que já vem no Windows 11).

Nenhuma biblioteca externa é necessária: só a biblioteca padrão do C e o que o próprio sistema operacional oferece.

### O Novo Mundo

```bash
cd novo-mundo
make
./novomundo                # no Windows: build.bat e depois jogar.bat
./novomundo --selftest     # os 226 testes automáticos
./novomundo --simulate 100 --speed 10   # o piloto automático joga 100 horas
```

### Os jogos da Parte III e o esqueleto

Cada pasta tem um `Makefile` e um `build.bat`:

```bash
cd conquista
make
./conquista                # no Windows: build.bat e depois conquista.exe
make simula                # o computador joga sozinho e imprime o resultado
```

O mesmo vale para `mercador/` e `colonia/`. A `batalha/` não tem simulação, e o `esqueleto/` tem `make simula` e `make teste`.

### Os exemplos dos capítulos

Os da Parte I usam só a biblioteca padrão:

```bash
cd exemplos/cap09
gcc -std=c11 -Wall -Wextra velha_ia.c -o velha_ia
./velha_ia
```

Os da Parte II usam o motor:

```bash
cd exemplos
gcc -std=c11 -Wall -Wextra -O2 cap22/mapa.c ../motor/*.c -o mapa -lm
./mapa
```

Os capítulos 13 e 14 têm uma pasta `hamurabi/` com o seu próprio `Makefile`.

---

## 🧭 Criando o seu próprio jogo

A pasta `esqueleto/` traz o **Posto Avançado**, um jogo pequeno, completo e funcionando, com mapa gerado por semente, economia em tempo real, eventos, salvamento seguro, piloto automático e testes. Cada lugar para mudar está marcado com `SEU JOGO:`. O `README.md` da pasta e o capítulo 46 do livro mostram o caminho.

---

## 📝 Licença

Os códigos-fonte deste repositório estão licenciados sob a **MIT License**: sinta-se à vontade para estudar, modificar e usar o motor, os jogos e o `esqueleto/` como base para os seus próprios jogos. O conteúdo textual, as ilustrações e a estrutura didática do livro **MAKE IN C: JOGOS DE ESTRATÉGIA** são protegidos por direitos autorais.

O Novo Mundo é inspirado no OGame, da Gameforge. OGame é marca dos seus respectivos donos, e este projeto não tem nenhuma ligação com eles.
