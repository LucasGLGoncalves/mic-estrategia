# NOVO MUNDO — simulador de colonização espacial

O projeto final do livro *Make in C: jogos de estratégia* (Parte IV, capítulos 30 a 44). Um jogo de estratégia espacial em **C puro com gráficos ASCII**, inspirado no OGame. Você funda um império num planeta gerado a partir do instante exato em que começou a jogar e do seu nome, expande minas, pesquisa tecnologias, constrói frotas, coloniza outros mundos, cria rotas comerciais e sobrevive a desastres naturais, piratas e invasores. A produção acontece **em tempo real, por segundo**, mesmo com o jogo fechado.

```
 _  _  _____   _____    __  __ _   _ _  _ ___   ___
| \| |/ _ \ \ / / _ \  |  \/  | | | | \| |   \ / _ \
| .` | (_) \ V / (_) | | |\/| | |_| | .` | |) | (_) |
|_|\_|\___/ \_/ \___/  |_|  |_|\___/|_|\_|___/ \___/
```

## Como jogar rapidamente

**Windows:** compile com `build.bat` e dê dois cliques em `jogar.bat` (ou rode `novomundo.exe`). Funciona no Windows Terminal e no console clássico do Windows 10/11.

**Linux / macOS:** `make` e depois `./novomundo`.

A janela ideal tem **120 colunas × 36 linhas**. O jogo tenta redimensionar o console clássico sozinho; no Windows Terminal, aumente a janela ou reduza a fonte (Ctrl + roda do mouse). Se a janela for menor, um aviso aparece no topo.

Opções de linha de comando:

| Opção | Efeito |
|---|---|
| `--ascii` | só caracteres ASCII (terminais antigos/fontes sem símbolos) |
| `--256` | força 256 cores |
| `--truecolor` | força cores de 24 bits (padrão no Windows) |
| `--selftest` | roda os 226 testes automáticos |
| `--simulate H --speed N` | simula H horas reais com o piloto automático e imprime a evolução |
| `--name NOME` | nome do comandante do piloto automático (com `--simulate`) |
| `--snapshot PASTA --hours H` | joga H horas com o piloto e grava todas as telas em arquivos `.cells` |
| `--gallery ARQUIVO` | grava uma galeria com um planeta de cada bioma |

## Compilando

| Plataforma | Comando |
|---|---|
| Linux / macOS / MSYS2 | `make` (testes: `make test`) |
| Windows com MinGW (MSYS2, w64devkit, TDM-GCC) | `build.bat` |
| Windows com Visual Studio | abra o *Developer Command Prompt* e rode `build.bat` |
| Manual (gcc) | `gcc -O2 -std=c11 src/*.c -o novomundo -lm` |

Não há dependências além da biblioteca padrão do C (e `windows.h` no Windows). O código é C11 e compila sem avisos no gcc e no MinGW com `-Wall -Wextra -pedantic`. No clang, acrescente `-Wno-missing-field-initializers`: as linhas das construções na tabela de itens deixam em zero, de propósito, os campos que só as naves usam (ataque, escudo, carga, velocidade).

## Teclas

| Tecla | Tela | Tecla | Tela |
|---|---|---|---|
| 1 | Visão Geral | C | Comércio (rotas e mercados) |
| 2 | Recursos | E | Mercador (trocas) |
| 3 | Civilização (população e alimento) | M | Mensagens |
| 4 | Instalações | T | Tecnologia (árvore, aplicações, info técnica, requisitos) |
| 5 | Pesquisa | O | Oficiais |
| 6 | Estaleiro | R | Classificação |
| 7 | Defesa | Z | Simulador de combate |
| 8 | Frota | H | Ajuda |
| 9 | Galáxia | Q | Salvar e sair |
| 0 | Império | V | Decidir evento pendente |

`[` `]` (ou `,` `.`) trocam de planeta, `L` alterna planeta/lua, `Esc` volta. Nas listas: `↑↓` escolhe, `Enter` constrói, `I` abre a info técnica do item, `X` cancela, `F` finaliza com Matéria Negra, `P` (em Recursos) muda a porcentagem de produção.

## Como o jogo funciona

### O universo nasce de uma semente
Ao fundar o império, a data e a hora são lidas na ordem **ano, dia, mês, minuto, hora, segundo** e combinadas com os caracteres do seu nome (FNV-1a + splitmix64). Essa semente gera o planeta natal (posição, diâmetro, campos, temperatura, bioma, atmosfera e pressão, clima, gravidade, duração do dia, água, fertilidade, radiação, magnetosfera, riqueza mineral e se há lua) e todo o universo: 552 impérios NPC com perfis diferentes (inativos, mineradores, defensivos, agressivos, mercantes, piratas e xenos) espalhados em 9 galáxias de 499 sistemas. A mesma semente sempre gera o mesmo universo.

Cada bioma tem sua própria paleta e textura na arte ASCII do planeta (temperado, oceânico, floresta tropical, desértico, savana, tundra, glacial, vulcânico com lava brilhando no lado noturno, tóxico, rochoso e lunar). O planeta gira, tem nuvens, atmosfera iluminada, luzes de cidades que aumentam com a população, lua, satélites e estação orbital em órbita.

### Tudo está interligado
As minas produzem por segundo com as fórmulas clássicas do OGame (metal `30·N·1,1^N`, cristal `20·N·1,1^N`, deutério dependente da temperatura), multiplicadas pela velocidade do universo, pela posição do planeta e pela riqueza mineral. Mas elas só rendem 100% se houver:

1. **energia** suficiente (Usina Solar, Reator de Fusão, Satélites Solares);
2. **trabalhadores**: cada nível de mina, usina e instalação exige gente;
3. a **população** precisa de **moradia** (Setor Residencial, Arcologia) e de **alimento**;
4. o alimento vem das **Fazendas Biosfera**, que consomem energia e dependem da fertilidade do planeta, ou do **Sintetizador de Alimentos**, que consome deutério;
5. sem comida a população diminui e pode se **revoltar**.

Falta de energia ou de trabalhadores também deixa obras e pesquisas mais lentas. Os armazéns limitam o estoque (e o jogo avisa quando um custo é maior que a capacidade).

### Árvore tecnológica
Todos os 75 itens têm requisitos (construções e pesquisas), com a árvore do OGame e acréscimos próprios: **Estação Orbital** (necessária para Astrofísica e Engenharia Orbital), **Porto Comercial**, **Biotecnologia**, **Medicina Avançada**, **Geofísica**, **Engenharia Orbital** e **Logística**. A tela Tecnologia mostra a árvore recursiva colorida (verde atendido, vermelho faltando), o que cada item libera, a tabela técnica por nível (produção, diferença, energia) e os requisitos diretos.

### Eventos, ameaça e rolagens de dado
Depois da proteção de novato (250 pontos) o universo reage ao seu crescimento. O indicador de **Ameaça** compara seus pontos e recursos expostos com suas defesas. Podem acontecer:

- **sondas de espionagem**, que podem ser abatidas por suas defesas e muitas vezes anunciam um ataque;
- **ataques de piratas, invasores e xenos**, com alerta e contagem regressiva (Tecnologia de Espionagem 4 ou a Falange revelam a composição);
- **mísseis interplanetários** (o Míssil Antibalístico intercepta);
- **desastres naturais** que dependem do planeta (terremotos em mundos vulcânicos, meteoros onde não há atmosfera, tempestades solares em planetas quentes de magnetosfera fraca, epidemias em selvas, tempestades de areia, nevascas, furacões, chuva ácida...);
- **acontecimentos bons**: imigrantes, veios minerais, cometas, naves abandonadas, mercadores viajantes, anomalias de Matéria Negra, descobertas científicas.

Os eventos mais sérios abrem uma **decisão**: gastar recursos por um bônus, aceitar um efeito garantido ou arriscar. O teste é **d20 + modificadores** (tecnologias e construções) contra uma Classe de Dificuldade; 20 natural é sucesso crítico e 1 é falha crítica. Dá para rolar de novo gastando Matéria Negra. Se você estiver ausente, o jogo escolhe a opção mais prudente.

### Combate
Simulação no estilo OGame, unidade por unidade: até 6 rodadas, alvos aleatórios, escudos que absorvem dano e se recarregam a cada rodada, chance de explosão quando a estrutura cai abaixo de 70%, **disparo rápido** com a tabela original, 30% de metal e cristal das naves destruídas viram **campo de destroços**, 70% das defesas são reparadas, saque de até 50% limitado pela carga, e grandes batalhas podem formar uma **lua**.

### Frotas, colônias e exploração
Missões: Atacar, Transportar, Estacionar, Espionar, Colonizar, Reciclar, Expedição (posição 16) e Explorar (revela bioma, tamanho, fertilidade e riqueza de posições vazias antes de colonizar). Tempo de voo e combustível seguem as fórmulas do OGame, com bônus dos motores e efeito da gravidade do planeta de origem. A Astrofísica libera colônias (posições extremas exigem níveis mais altos) e expedições; outras galáxias exigem Propulsor Hiperespacial 3. Posições 1-3 favorecem cristal e energia solar, 6-10 favorecem metal e 13-15 favorecem deutério, então vale especializar colônias.

### Comércio
Rotas **internas** levam recursos continuamente entre seus planetas (por exemplo, alimento de um mundo fértil para um árido). Rotas com **mundos Mercantes** exportam um recurso e importam outro, com taxas que flutuam com o mercado e com o que cada mercante deseja. Piratas emboscam comboios: atribua naves militares como escolta para reduzir o risco e as perdas. O **Mercador** faz trocas imediatas na base 3:2:1 com 10% de taxa (zero quando um mercador viajante aparece).

### Luas, oficiais e Matéria Negra
Luas têm construções próprias: Base Lunar (campos), Falange de Sensores (aviso antecipado de ataques), Portal de Salto (tecla `J` na Frota, entre luas) e Extrator de Hélio-3 (deutério sem energia). Oficiais custam Matéria Negra e duram 7 dias reais. A Matéria Negra vem de expedições, anomalias e das **missões guiadas** (canto superior esquerdo), que ensinam o caminho do império.

### Progresso offline
Ao reabrir o jogo, tudo o que aconteceu enquanto você esteve fora é processado em ordem cronológica: produção, filas de obras, estaleiro, pesquisas, frotas, rotas e eventos (com frequência reduzida e ataques limitados durante a ausência). Um relatório mostra o que foi produzido. O jogo salva a cada 30 segundos e ao sair, de forma atômica, em `novo_mundo.sav` na pasta de onde foi executado.

## Estrutura do código

| Arquivo | Conteúdo |
|---|---|
| `src/term.c` | camada de terminal portátil (console do Windows com VT e UTF-16 / termios POSIX), canvas com cores de 24 bits e redesenho incremental |
| `src/util.c` | gerador xoshiro256**, hashes, ruído 3D, UTF-8, formatação pt-BR |
| `src/data.c` | definição dos 75 itens e da tabela de disparo rápido |
| `src/economy.c` | custos, tempos, produção, energia, trabalhadores, filas, pontos |
| `src/sim.c` | avanço do tempo (online e offline), filas, população e alimento |
| `src/universe.c` | geração procedural de planetas e impérios NPC |
| `src/fleet.c` | missões, voo, combustível, expedições, colonização, portal de salto, mísseis |
| `src/combat.c` | simulador de combate |
| `src/events.c` | eventos aleatórios, decisões com d20, ameaça, frotas hostis |
| `src/trade.c` | rotas comerciais e mercador |
| `src/save.c` | salvamento em texto versionado |
| `src/art.c` | planetas em ASCII, paisagens com construções, naves |
| `src/ui.c`, `src/ui_screens.c` | interface |
| `src/ai.c` | piloto automático usado nos testes e simulações |
| `src/tests.c` | testes automáticos |
| `tools/` | `pty_play.py` (joga o binário real num pseudo-terminal e confere as telas) e `render_cells.py` (transforma as telas gravadas pelo `--snapshot` em PNG) |

## Licença e créditos

Código sob a licença MIT (veja `LICENSE` na raiz do repositório). Novo Mundo é inspirado no OGame, da Gameforge; OGame é marca registrada dos seus donos, e este projeto não tem ligação com eles. As fórmulas clássicas do gênero foram reescritas do zero, a partir da sua descrição pública pela comunidade.
