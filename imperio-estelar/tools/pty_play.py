#!/usr/bin/env python3
"""Drive the real game binary inside a pseudo-terminal, like a human player.
Keys are sent with delays; the ANSI output is fed to the pyte emulator so we can
assert on what the player actually sees."""
import os, pty, select, sys, time, signal, shutil, tempfile
import pyte

BIN = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "./imperio")
EXTRA = sys.argv[2:]
W, H = 120, 36

UP, DOWN, LEFT, RIGHT = "\x1b[A", "\x1b[B", "\x1b[D", "\x1b[C"
ENTER, ESC, TAB, BS = "\r", "\x1b", "\t", "\x7f"


class Game:
    def __init__(self, cwd):
        self.screen = pyte.Screen(W, H)
        self.stream = pyte.ByteStream(self.screen)
        pid, fd = pty.fork()
        if pid == 0:
            os.chdir(cwd)
            os.environ["TERM"] = "xterm-256color"
            os.environ["COLORTERM"] = "truecolor"
            os.execv(BIN, [BIN] + EXTRA)
        self.pid, self.fd = pid, fd
        import fcntl, termios, struct
        fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", H, W, 0, 0))
        self.bytes = 0

    def pump(self, secs):
        end = time.time() + secs
        while time.time() < end:
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                try:
                    data = os.read(self.fd, 65536)
                except OSError:
                    return False
                if not data:
                    return False
                self.bytes += len(data)
                self.stream.feed(data)
        return True

    def send(self, s, wait=0.25):
        os.write(self.fd, s.encode("utf-8"))
        self.pump(wait)

    def text(self):
        return "\n".join(self.screen.display)

    def has(self, s):
        return s in self.text()

    def alive(self):
        try:
            pid, status = os.waitpid(self.pid, os.WNOHANG)
            return pid == 0
        except ChildProcessError:
            return False

    def wait_exit(self, secs=5):
        end = time.time() + secs
        while time.time() < end:
            self.pump(0.1)
            try:
                pid, status = os.waitpid(self.pid, os.WNOHANG)
                if pid != 0:
                    return status
            except ChildProcessError:
                return 0
        return None


fails = 0


def check(cond, msg, g=None):
    global fails
    print(("  ok   " if cond else "  FAIL ") + msg)
    if not cond:
        fails += 1
        if g:
            print(g.text())


def main():
    tmp = tempfile.mkdtemp()
    print("[1] registro e revelação do planeta")
    g = Game(tmp)
    g.pump(1.0)
    check(g.has("Fundar novo império"), "tela de registro aparece", g)
    g.send("Lucas Gonçalves")
    g.send(ENTER)
    g.send(BS * 13 + "Terra Nova")
    g.send(ENTER)
    g.send(RIGHT)  # speed x25
    g.send(ENTER)
    g.send(ENTER, 0.8)
    check(g.has("A semente do seu universo"), "tela de semente", g)
    check(g.has("Lucas Gonçalves"), "nome com acento preservado", g)
    g.send(ENTER, 0.8)
    check(g.has("Visão Geral - Terra Nova"), "visão geral do planeta natal", g)
    check(g.has("x25"), "velocidade escolhida", g)
    check(g.has("Dados planetários"), "painel de dados", g)
    print("[2] construir mina e usina")
    g.send("2", 0.4)
    check(g.has("Recursos - Terra Nova"), "tela de recursos", g)
    g.send(DOWN * 3, 0.3)  # solar
    g.send(ENTER, 0.4)
    g.send(UP * 3, 0.3)
    g.send(ENTER, 0.6)
    check(g.has("Fila 2/2") or g.has("Em obra") or g.has("Na fila"), "obras na fila", g)
    print("[3] navegar por todas as telas")
    for k, title in [("1", "Visão Geral"), ("3", "Civilização"), ("4", "Instalações"), ("5", "Pesquisa"), ("6", "Estaleiro"),
                     ("7", "Defesa"), ("8", "Frota"), ("9", "Galáxia"), ("0", "Império"), ("c", "Comércio"), ("e", "Mercador"),
                     ("m", "Mensagens"), ("t", "Tecnologia"), ("o", "Oficiais"), ("r", "Classificação"), ("z", "Simulador"), ("h", "Ajuda")]:
        g.send(k, 0.35)
        check(g.has(title), f"tela '{title}' via tecla {k}", g if not g.has(title) else None)
    print("[4] tecnologia: abas")
    g.send("t", 0.3)
    for _ in range(4):
        g.send(RIGHT, 0.3)
    check(g.has("Árvore") and g.has("Requisitos"), "abas de tecnologia", g)
    print("[5] galáxia: navegar e abrir ações")
    g.send("9", 0.3)
    g.send(RIGHT, 0.3)
    g.send(DOWN, 0.2)
    g.send(ENTER, 0.4)
    check(g.has("Ações em"), "menu de ações", g)
    g.send(ESC, 0.3)
    print("[6] mercador: troca")
    g.send("e", 0.3)
    g.send(ENTER, 0.3)
    check(g.has("Quanto Metal vender"), "prompt de quantidade", g)
    g.send(BS * 6 + "100", 0.2)
    g.send(ENTER, 0.5)
    check(g.has("Trocado") or g.has("Cristal"), "troca efetuada", g)
    print("[7] renomear planeta com acentos")
    g.send("1", 0.3)
    g.send("n", 0.3)
    g.send(BS * 20 + "Éden Ômega", 0.3)
    g.send(ENTER, 0.5)
    check(g.has("Visão Geral - Éden Ômega"), "renomeado", g)
    print("[8] redimensionar e ajuda")
    g.send("h", 0.3)
    g.send(RIGHT, 0.3)
    check(g.has("TECLAS"), "ajuda página 2", g)
    print("[9] sair salvando")
    g.send("q", 0.4)
    check(g.has("Salvar o império e sair"), "confirmação de saída", g)
    g.send("s", 0.2)
    st = g.wait_exit(5)
    check(st is not None, "processo terminou", g)
    check(os.path.exists(os.path.join(tmp, "imperio_estelar.sav")), "arquivo salvo")
    print(f"    bytes recebidos do jogo: {g.bytes}")

    print("[10] recarregar: tela de título + progresso offline")
    time.sleep(2.5)
    g2 = Game(tmp)
    g2.pump(1.0)
    check(g2.has("Continuar - Comandante Lucas Gonçalves"), "título oferece continuar", g2)
    g2.send(ENTER, 1.5)
    check(g2.has("Éden Ômega"), "planeta carregado com nome novo", g2)
    g2.send("2", 0.4)
    check(g2.has("Recursos - Éden Ômega"), "telas funcionam após carregar", g2)
    g2.send("\x03", 0.4)  # ctrl+c -> confirm
    check(g2.has("Salvar o império e sair"), "Ctrl+C pede confirmação", g2)
    g2.send("n", 0.3)
    os.kill(g2.pid, signal.SIGTERM)
    st = g2.wait_exit(5)
    check(st is not None, "SIGTERM encerra salvando", g2)

    print("[11] modo --ascii (terminais antigos)")
    global EXTRA
    EXTRA = ["--ascii"]
    g3 = Game(tmp)
    g3.pump(1.0)
    g3.send(ENTER, 1.5)
    for k in "1234567890cemtorzh":
        g3.send(k, 0.25)
        nonascii = [ch for ch in g3.text() if ord(ch) > 126]
        if nonascii:
            break
    check(not nonascii, f"somente ASCII na tela (achou {set(nonascii) if nonascii else ''})", g3 if nonascii else None)
    g3.send("q", 0.3)
    g3.send("s", 0.3)
    check(g3.wait_exit(5) is not None, "saiu no modo ascii")
    shutil.rmtree(tmp, ignore_errors=True)
    print(f"\n{'TUDO OK' if fails == 0 else str(fails) + ' FALHAS'}")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
