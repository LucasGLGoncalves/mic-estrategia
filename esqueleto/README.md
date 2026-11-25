# Posto Avançado: o esqueleto do seu jogo

Este é o ponto de partida do capítulo 46 de *Make in C: jogos de estratégia*. É um jogo pequeno, completo e funcionando: um posto avançado num mapa gerado por semente, com três recursos, quatro construções, população que cresce e passa fome, eventos sorteados, salvamento automático, progresso com o jogo fechado, um piloto automático e testes.

Ele não foi feito para ser divertido por muito tempo. Foi feito para ser **transformado**: cada parte que você vai querer trocar está marcada no código com o comentário `SEU JOGO:`.

## Compilar e jogar

No Linux, no macOS, no MSYS2 ou no w64devkit:

```sh
make
./posto
```

No Windows, sem `make`: dê dois cliques em `build.bat` e depois rode `posto.exe` (de preferência no Windows Terminal, com a janela de pelo menos 100 por 30).

| Tecla | O que faz |
|---|---|
| setas | movem o cursor no mapa |
| 1 a 4 | escolhem a construção (casa, fazenda, serraria, pedreira) |
| Enter ou espaço | constroem na quadra do cursor |
| D | demole (metade do custo volta) |
| V | muda a velocidade: 1x, 10x, 100x |
| Esc | salva e sai |

| Opção | O que faz |
|---|---|
| `--novo` | começa um posto novo, mesmo que haja um salvo |
| `--semente N` | escolhe o mapa (com `--novo` ou `--simula`) |
| `--auto H` | o piloto joga as primeiras H horas e depois passa o comando |
| `--simula H` | o piloto joga H horas, sem tela, e imprime a evolução |
| `--testa` | roda os testes automáticos |

`make teste` e `make simula` são atalhos para as duas últimas.

## Os arquivos

| Arquivo | O que tem |
|---|---|
| `jogo.h`, `jogo.c` | o estado e as regras: tabelas, mapa, produção, fome, eventos, construir e demolir. **Nenhuma tela.** |
| `telas.h`, `telas.c` | o desenho e as teclas. Lê o jogo e chama as suas funções. |
| `salvar.h`, `salvar.c` | o arquivo salvo, em texto, com assinatura e troca atômica. |
| `piloto.h`, `piloto.c` | um jogador automático de regras simples. |
| `testes.c` | os testes, com a macro `CHECK`. |
| `main.c` | as opções, o tempo fora e o laço do jogo. |

O jogo usa o motor da Parte II do livro, na pasta `../motor`. Se você copiar o esqueleto para outro lugar, copie o motor junto e ajuste os caminhos em `Makefile`, `build.bat` e nos `#include "../motor/..."`.

## Como transformar no seu jogo

1. **Dê um nome.** Troque `posto` no `Makefile` e no `build.bat`, o título no cabeçalho (`telas.c`) e o nome do arquivo salvo (`salvar.h`).
2. **Troque as tabelas.** Recursos, terrenos e construções estão em `jogo.h` (os `enum`) e em `jogo.c` (as tabelas). Uma construção nova é um valor no `enum` e uma linha na tabela.
3. **Mude o mundo.** `jogo_gera_mapa` decide o terreno a partir da semente. Experimente outras camadas de ruído, rios, recursos raros.
4. **Mude as regras.** `passo` (em `jogo.c`) é o coração do tempo: produção, consumo, crescimento, eventos.
5. **Escreva os eventos.** Os de agora não têm escolhas. Transforme-os em decisões em que toda opção tem um preço (capítulos 5, 40 e 45).
6. **Salve o que for novo.** Cada campo novo da estrutura `Jogo` precisa de uma linha em `salva_jogo` e outra em `carrega_jogo`. O teste de ida e volta avisa se você esquecer.
7. **Teste.** Cada regra nova merece um `CHECK`. Rode `make teste` depois de cada mudança.
8. **Equilibre.** Ensine o piloto a jogar o seu jogo e use `--simula` para ver se a economia anda no ritmo que você quer.

Bom jogo, e bom código.
