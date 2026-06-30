#include "raylib.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <iostream>
#include <cstring>

using namespace std;

static inline float Clamp(float value, float minValue, float maxValue) {
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

// ───────────────────────────────────────────────────────────────
//  Constantes
// ───────────────────────────────────────────────────────────────
static const int SCREEN_W   = 1360;
static const int SCREEN_H   = 768;
static const int TILE        = 16;   // tamanho do tile em pixels
static const int SCALE       = 3;    // fator de escala (16*3 = 48px na tela)
static const int TS          = TILE * SCALE;  // tamanho real na tela = 48

static const int MAP_W = 20;
static const int MAP_H = 16;

// Paleta inspirada na tela inicial: sombra, brasa e dourado
static const Color C_BG       = {8,   5,   15,  255};
static const Color C_WALL     = {46,  23,  18,  255};
static const Color C_WALL_HI  = {92,  42,  22,  255};
static const Color C_WALL_LO  = {20,  10,  16,  255};
static const Color C_FLOOR    = {19,  13,  25,  255};
static const Color C_FLOOR_HI = {35,  20,  32,  255};
static const Color C_FLOOR_LO = {10,  7,   18,  255};
static const Color C_PLAYER   = {240, 145, 55,  255};
static const Color C_ENEMY    = {205, 55,  35,  255};
static const Color C_ITEM     = {255, 190, 70,  255};
static const Color C_PORTAL   = {220, 88,  25,  255};
static const Color C_HP_BAR   = {90,  210, 80,  255};
static const Color C_HP_BG    = {76,  18,  13,  255};
static const Color C_MP_BAR   = {185, 105, 230, 255};
static const Color C_MP_BG    = {28,  15,  38,  255};
static const Color C_UI_BG    = {13,  8,   20,  238};
static const Color C_UI_BG2   = {31,  15,  20,  238};
static const Color C_UI_BORD  = {170, 70,  20,  255};
static const Color C_TEXT     = {245, 220, 180, 255};
static const Color C_TEXT_DIM = {160, 120, 80,  255};
static const Color C_GOLD     = {255, 180, 60,  255};
static const Color C_DAMAGE   = {255, 75,  40,  255};
static const Color C_HEAL     = {125, 240, 105, 255};

// ───────────────────────────────────────────────────────────────
//  Util visuais
// ───────────────────────────────────────────────────────────────
static inline unsigned hashTile(int x, int y, int seed) {
    unsigned h = (unsigned)(x * 374761393 + y * 668265263 + seed * 2654435761u);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static inline Color colorLerp(Color a, Color b, float t) {
    t = Clamp(t, 0.0f, 1.0f);
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static inline Color shade(Color c, float f) {
    return Color{
        (unsigned char)Clamp(c.r * f, 0, 255),
        (unsigned char)Clamp(c.g * f, 0, 255),
        (unsigned char)Clamp(c.b * f, 0, 255),
        c.a
    };
}

// Painel com gradiente vertical sutil + borda dupla + cantos arredondados
static void drawPanel(Rectangle r, Color top, Color bottom, Color border, float roundness = 0.12f) {
    DrawRectangleGradientV((int)r.x, (int)r.y, (int)r.width, (int)r.height, top, bottom);
    DrawRectangleRoundedLines(r, roundness, 8, border);
    Rectangle inner = {r.x+2, r.y+2, r.width-4, r.height-4};
    Color innerB = border; innerB.a = 90;
    DrawRectangleRoundedLines(inner, roundness, 8, innerB);
}

// ───────────────────────────────────────────────────────────────
//  Mapa (0=chão, 1=parede, 2=portal, 3=item)
// ───────────────────────────────────────────────────────────────
static int mapData[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,1,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,1,1,0,1},
    {1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,1,0,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,1,0,1},
    {1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1},
    {1,0,0,1,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

// ───────────────────────────────────────────────────────────────
//  Log de mensagens
// ───────────────────────────────────────────────────────────────
struct LogEntry { string texto; Color cor; float timer; };
static vector<LogEntry> gLog;

void logMsg(const string& txt, Color cor = C_TEXT) {
    gLog.push_back({txt, cor, 4.0f});
    if ((int)gLog.size() > 6) gLog.erase(gLog.begin());
}

// ───────────────────────────────────────────────────────────────
//  Partículas (ambientais: poeira, faíscas de portal, brilho de item)
// ───────────────────────────────────────────────────────────────
struct Particle {
    float x, y, vx, vy, life, maxLife, size;
    Color cor;
};
static vector<Particle> gParticles;

static void spawnParticle(float x, float y, float vx, float vy, float life, float size, Color c) {
    if (gParticles.size() > 400) return;
    gParticles.push_back({x, y, vx, vy, life, life, size, c});
}

static void updateParticles(float dt) {
    for (auto& p : gParticles) {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
    }
    gParticles.erase(remove_if(gParticles.begin(), gParticles.end(),
        [](const Particle& p){ return p.life <= 0; }), gParticles.end());
}

// ───────────────────────────────────────────────────────────────
//  Item no chão / Inventário
// ───────────────────────────────────────────────────────────────
struct Item {
    string nome, tipo;
    int valor;
};

// ───────────────────────────────────────────────────────────────
//  Entidade base (jogador e inimigos)
// ───────────────────────────────────────────────────────────────
struct Entidade {
    float x, y;           // posição no mapa (tiles)
    int   hp, hpMax;
    int   mp, mpMax;
    int   atk, def;
    int   nivel;
    bool  vivo = true;
    Color cor;
    string nome;

    // animação
    float animTimer  = 0;
    float shakeTimer = 0;
    int   shakeDir   = 1;
    float flashTimer = 0; // flash branco ao tomar dano

    void receberDano(int d) {
        int real = max(1, d - def);
        hp = max(0, hp - real);
        shakeTimer = 0.25f;
        shakeDir   = (GetRandomValue(0,1) ? 1 : -1);
        flashTimer = 0.15f;
        if (hp == 0) vivo = false;
        logMsg(nome + " recebeu " + to_string(real) + " dano!", C_DAMAGE);
    }

    void curar(int v) {
        int ant = hp;
        hp = min(hp + v, hpMax);
        logMsg(nome + " recuperou " + to_string(hp - ant) + " HP!", C_HEAL);
    }

    Rectangle getRect() const {
        float sx = shakeTimer > 0 ? shakeDir * 2.0f : 0;
        return { x * TS + sx, y * TS, (float)TS, (float)TS };
    }
};

// ───────────────────────────────────────────────────────────────
//  Inimigo
// ───────────────────────────────────────────────────────────────
struct Inimigo : Entidade {
    int expRecomp, ouroRecomp;
    float moveTimer = 0;
    float moveDelay = 1.2f; // segundos entre movimentos

    void tick(float dt, const Entidade& jogador, int map[MAP_H][MAP_W]) {
        if (!vivo) return;
        animTimer  += dt;
        shakeTimer  = max(0.0f, shakeTimer - dt);
        flashTimer  = max(0.0f, flashTimer - dt);
        moveTimer  += dt;

        if (moveTimer < moveDelay) return;
        moveTimer = 0;

        float dx = jogador.x - x;
        float dy = jogador.y - y;
        float dist = abs(dx) + abs(dy);

        int nx = (int)x, ny = (int)y;

        if (dist <= 6) {
            if (abs(dx) > abs(dy))  nx += (dx > 0 ? 1 : -1);
            else                     ny += (dy > 0 ? 1 : -1);
        } else {
            int dir = GetRandomValue(0, 3);
            if (dir == 0) ny--;
            if (dir == 1) ny++;
            if (dir == 2) nx--;
            if (dir == 3) nx++;
        }

        if (nx >= 0 && nx < MAP_W && ny >= 0 && ny < MAP_H && map[ny][nx] == 0) {
            x = (float)nx;
            y = (float)ny;
        }
    }
};

// ───────────────────────────────────────────────────────────────
//  Habilidade do jogador
// ───────────────────────────────────────────────────────────────
struct Habilidade {
    string nome, desc;
    int    custoMp, multPct;
};

// ───────────────────────────────────────────────────────────────
//  Estado do jogo
// ───────────────────────────────────────────────────────────────
enum class GameState {
    MENU, CHAR_SELECT, PLAYING, BATTLE, INVENTORY, SKILL_MENU, PAUSE, GAME_OVER, VICTORY
};

// ───────────────────────────────────────────────────────────────
//  Estrutura de Batalha
// ───────────────────────────────────────────────────────────────
struct BattleState {
    Inimigo* alvo = nullptr;
    bool     jogadorVez = true;
    float    delay      = 0;
    bool     acabou     = false;
    bool     vitoria    = false;

    struct FloatText { string txt; float x, y, timer; Color cor; };
    vector<FloatText> floatTexts;

    void addFloat(const string& t, float x, float y, Color c) {
        floatTexts.push_back({t, x, y, 1.5f, c});
    }
};

// ───────────────────────────────────────────────────────────────
//  Classe principal do jogo
// ───────────────────────────────────────────────────────────────
struct Game {
    GameState     state  = GameState::MENU;
    int           andar  = 1;
    int           ouro   = 50;

    Entidade      jogador;
    string        classeNome;
    vector<Habilidade> habilidades;
    vector<Item>       inventario;
    int                exp      = 0;
    int                expProx  = 100;

    int           map[MAP_H][MAP_W];
    vector<Inimigo>    inimigos;
    vector<pair<int,int>> itensNoMapa;

    float camX = 0, camY = 0;

    BattleState   battle;

    int menuSel   = 0;
    int classSel  = 0;
    int invSel    = 0;
    int skillSel  = 0;
    int pauseSel  = 0;
    GameState pauseReturn = GameState::PLAYING;

    float flashTimer = 0;
    Color flashColor = WHITE;

    float globalTime = 0;
    int   mapSeed = 1;

    // ── Inicialização ──────────────────────────────────────────
    void init() {
        srand((unsigned)time(nullptr));
        memcpy(map, mapData, sizeof(map));
        gerarMapa();
    }

    void criarJogador(int cls) {
        jogador = {};
        jogador.x = 2; jogador.y = 2;
        jogador.nivel = 1;
        habilidades.clear();
        inventario.clear();

        if (cls == 0) {
            classeNome      = "Guerreiro";
            jogador.nome    = "Heroi";
            jogador.hpMax   = 160; jogador.hp = 160;
            jogador.mpMax   = 40;  jogador.mp = 40;
            jogador.atk     = 20;  jogador.def = 14;
            jogador.cor     = C_PLAYER;
            habilidades.push_back({"Golpe Pesado",    "Dano fisico forte",  10, 160});
            habilidades.push_back({"Escudo Quebrado", "Ignora parte da DEF",15, 130});
            habilidades.push_back({"Frenesi",         "Ataque devastador",  25, 220});
            inventario.push_back({"Pocao de Vida",  "cura_vida", 60});
            inventario.push_back({"Elixir de Forca","buff_atk",  10});
        } else if (cls == 1) {
            classeNome      = "Mago";
            jogador.nome    = "Heroi";
            jogador.hpMax   = 90;  jogador.hp = 90;
            jogador.mpMax   = 130; jogador.mp = 130;
            jogador.atk     = 32;  jogador.def = 4;
            jogador.cor     = {180, 100, 255, 255};
            habilidades.push_back({"Bola de Fogo",  "Conjuracao ardente",   20, 180});
            habilidades.push_back({"Raio Arcano",   "Energia pura",         30, 260});
            habilidades.push_back({"Tempestade",    "Devastacao em area",   55, 380});
            habilidades.push_back({"Drenar Magia",  "Rouba forca magica",    8,  90});
            inventario.push_back({"Pocao de Mana",     "cura_mana", 80});
            inventario.push_back({"Pergaminho Arcano", "buff_atk",  15});
        } else {
            classeNome      = "Ladino";
            jogador.nome    = "Heroi";
            jogador.hpMax   = 110; jogador.hp = 110;
            jogador.mpMax   = 70;  jogador.mp = 70;
            jogador.atk     = 24;  jogador.def = 6;
            jogador.cor     = {100, 255, 160, 255};
            habilidades.push_back({"Facada",      "Ataque furtivo",    15, 210});
            habilidades.push_back({"Veneno",      "Dano continuo",     10, 120});
            habilidades.push_back({"Golpe Duplo", "Dois golpes rapidos",20, 170});
            inventario.push_back({"Pocao de Vida", "cura_vida", 50});
            inventario.push_back({"Faca Afiada",   "buff_atk",   8});
        }
        jogador.vivo = true;
    }

    void gerarMapa() {
        memcpy(map, mapData, sizeof(map));
        inimigos.clear();
        itensNoMapa.clear();
        mapSeed = GetRandomValue(1, 1000000);

        map[14][18] = 2;

        for (int i = 0; i < 3; i++) {
            int tx, ty;
            do { tx = GetRandomValue(1, MAP_W-2); ty = GetRandomValue(1, MAP_H-2); }
            while (map[ty][tx] != 0);
            map[ty][tx] = 3;
            itensNoMapa.push_back({tx, ty});
        }

        int numInimigos = min(2 + andar, 8);
        for (int i = 0; i < numInimigos; i++) {
            Inimigo e{};
            do { e.x = (float)GetRandomValue(2, MAP_W-3);
                 e.y = (float)GetRandomValue(2, MAP_H-3); }
            while (map[(int)e.y][(int)e.x] != 0);

            if (andar <= 2) {
                e.nome = "Goblin";   e.cor = {180, 80, 80, 255};
                e.hpMax = 55; e.hp = 55; e.mp = 0; e.mpMax = 0;
                e.atk = 9;  e.def = 1; e.nivel = 1;
                e.expRecomp = 25; e.ouroRecomp = 8;
            } else if (andar <= 4) {
                e.nome = "Orc";      e.cor = {80, 160, 80, 255};
                e.hpMax = 130; e.hp = 130; e.mp = 0; e.mpMax = 0;
                e.atk = 16; e.def = 9; e.nivel = andar;
                e.expRecomp = 60; e.ouroRecomp = 20;
                e.moveDelay = 1.8f;
            } else if (andar <= 6) {
                e.nome = "Necromante"; e.cor = {140, 60, 200, 255};
                e.hpMax = 95; e.hp = 95; e.mp = 80; e.mpMax = 80;
                e.atk = 22; e.def = 4; e.nivel = andar;
                e.expRecomp = 120; e.ouroRecomp = 55;
                e.moveDelay = 0.9f;
            } else {
                e.nome = "Dragao";   e.cor = {255, 120, 30, 255};
                e.hpMax = 400; e.hp = 400; e.mp = 120; e.mpMax = 120;
                e.atk = 38; e.def = 16; e.nivel = andar;
                e.expRecomp = 300; e.ouroRecomp = 200;
                e.moveDelay = 2.0f;
                numInimigos = 1;
            }
            e.vivo = true;
            inimigos.push_back(e);
            if (andar >= 7) break;
        }
    }

    void atualizarCamera() {
        float alvoX = jogador.x * TS - SCREEN_W / 2.0f + TS / 2.0f;
        float alvoY = jogador.y * TS - SCREEN_H / 2.0f + TS / 2.0f;
        camX += (alvoX - camX) * 0.12f;
        camY += (alvoY - camY) * 0.12f;
        camX = max(0.0f, min(camX, (float)(MAP_W * TS - SCREEN_W)));
        camY = max(0.0f, min(camY, (float)(MAP_H * TS - SCREEN_H)));
    }

    void moverJogador() {
        int nx = (int)jogador.x, ny = (int)jogador.y;
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    ny--;
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  ny++;
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  nx--;
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) nx++;

        if (nx == (int)jogador.x && ny == (int)jogador.y) return;

        if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) return;
        if (map[ny][nx] == 1) return;

        for (auto& e : inimigos) {
            if (e.vivo && (int)e.x == nx && (int)e.y == ny) {
                iniciarBatalha(&e);
                return;
            }
        }

        if (map[ny][nx] == 2) {
            andar++;
            if (andar > 7) { state = GameState::VICTORY; return; }
            logMsg("Andar " + to_string(andar) + "!", C_PORTAL);
            gerarMapa();
            jogador.x = 2; jogador.y = 2;
            flashTimer = 0.5f; flashColor = C_PORTAL;
            return;
        }

        if (map[ny][nx] == 3) {
            map[ny][nx] = 0;
            int tipo = GetRandomValue(0, 2);
            if (tipo == 0)      inventario.push_back({"Pocao de Vida",  "cura_vida", 50});
            else if (tipo == 1) inventario.push_back({"Pocao de Mana",  "cura_mana", 50});
            else                inventario.push_back({"Elixir de Forca","buff_atk",  8});
            logMsg("Item coletado!", C_ITEM);
            flashTimer = 0.2f; flashColor = C_ITEM;
            for (int i = 0; i < 18; i++) {
                float ang = (float)GetRandomValue(0, 360) * DEG2RAD;
                float spd = (float)GetRandomValue(20, 60);
                spawnParticle(nx*TS+TS/2.0f, ny*TS+TS/2.0f, cosf(ang)*spd, sinf(ang)*spd, 0.6f, 3.0f, C_ITEM);
            }
        }

        jogador.x = (float)nx;
        jogador.y = (float)ny;
    }

    void iniciarBatalha(Inimigo* e) {
        battle         = {};
        battle.alvo    = e;
        battle.jogadorVez = true;
        state          = GameState::BATTLE;
        logMsg("Batalha com " + e->nome + "!", C_DAMAGE);
    }

    void batalhaAcaoJogador(int acao) {
        if (!battle.jogadorVez || battle.acabou) return;

        Inimigo* alvo = battle.alvo;
        int dano = 0;
        string nomeAcao = "Ataque";

        if (acao == 0) {
            dano = jogador.atk + GetRandomValue(-2, 3);
            if (GetRandomValue(1,100) <= 15) { dano *= 2; nomeAcao = "CRITICO!"; }
        } else {
            int idx = acao - 1;
            if (idx < (int)habilidades.size()) {
                auto& h = habilidades[idx];
                if (jogador.mp < h.custoMp) {
                    logMsg("Mana insuficiente!", C_DAMAGE); return;
                }
                jogador.mp -= h.custoMp;
                dano = jogador.atk * h.multPct / 100;
                nomeAcao = h.nome;
            }
        }

        alvo->receberDano(dano);
        battle.addFloat("-" + to_string(max(1, dano - alvo->def)), alvo->x * TS - camX + TS/2, alvo->y * TS - camY, C_DAMAGE);

        if (!alvo->vivo) {
            battle.acabou  = true;
            battle.vitoria = true;
            ganharExpOuro(alvo->expRecomp, alvo->ouroRecomp);
            flashTimer = 0.3f; flashColor = C_GOLD;
        } else {
            battle.jogadorVez = false;
            battle.delay      = 1.0f;
        }
    }

    void batalhaAcaoInimigo(float dt) {
        if (battle.jogadorVez || battle.acabou) return;
        battle.delay -= dt;
        if (battle.delay > 0) return;

        Inimigo* e = battle.alvo;
        int dano = e->atk + GetRandomValue(-2, 3);

        if (classeNome == "Ladino" && GetRandomValue(1,100) <= 28) {
            logMsg("Evasao!", C_HEAL);
        } else {
            jogador.receberDano(dano);
            battle.addFloat("-" + to_string(max(1, dano - jogador.def)),
                jogador.x * TS - camX + TS/2, jogador.y * TS - camY, C_DAMAGE);
        }

        if (!jogador.vivo) {
            battle.acabou  = true;
            battle.vitoria = false;
            state = GameState::GAME_OVER;
        } else {
            battle.jogadorVez = true;
        }
    }

    void encerrarBatalha() {
        if (!battle.vitoria) return;
        battle.alvo->vivo = false;
        state = GameState::PLAYING;
        battle = {};
    }

    void ganharExpOuro(int e, int o) {
        ouro += o;
        exp  += e;
        logMsg("+" + to_string(e) + " EXP  +" + to_string(o) + " ouro", C_GOLD);
        while (exp >= expProx) {
            exp -= expProx;
            expProx = 100 * (jogador.nivel + 1);
            jogador.nivel++;
            jogador.hpMax += 20; jogador.hp = jogador.hpMax;
            jogador.mpMax += 15; jogador.mp = jogador.mpMax;
            jogador.atk   += 5;
            jogador.def   += 2;
            logMsg("LEVEL UP! Nivel " + to_string(jogador.nivel) + "!", C_GOLD);
            flashTimer = 0.6f; flashColor = C_GOLD;
        }
    }

    // ── Update ─────────────────────────────────────────────────
    void update(float dt) {
        globalTime += dt;
        flashTimer = max(0.0f, flashTimer - dt);
        updateParticles(dt);

        for (auto& l : gLog) l.timer -= dt;
        gLog.erase(remove_if(gLog.begin(), gLog.end(),
            [](const LogEntry& l){ return l.timer <= 0; }), gLog.end());

        for (auto& f : battle.floatTexts) { f.y -= 30 * dt; f.timer -= dt; }
        battle.floatTexts.erase(remove_if(battle.floatTexts.begin(), battle.floatTexts.end(),
            [](const BattleState::FloatText& f){ return f.timer <= 0; }), battle.floatTexts.end());

        switch (state) {
        case GameState::MENU:
            if (IsKeyPressed(KEY_UP))   menuSel = max(0, menuSel - 1);
            if (IsKeyPressed(KEY_DOWN)) menuSel = min(1, menuSel + 1);
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (menuSel == 0) state = GameState::CHAR_SELECT;
                else              CloseWindow();
            }
            break;

        case GameState::CHAR_SELECT:
            if (IsKeyPressed(KEY_LEFT))  classSel = max(0, classSel - 1);
            if (IsKeyPressed(KEY_RIGHT)) classSel = min(2, classSel + 1);
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                criarJogador(classSel);
                gerarMapa();
                state = GameState::PLAYING;
                logMsg("Bem-vindo, " + classeNome + "!", C_HEAL);
            }
            if (IsKeyPressed(KEY_ESCAPE)) state = GameState::MENU;
            break;

        case GameState::PLAYING:
            moverJogador();
            atualizarCamera();
            for (auto& e : inimigos)
                e.tick(dt, jogador, map);
            jogador.shakeTimer = max(0.0f, jogador.shakeTimer - dt);
            jogador.flashTimer = max(0.0f, jogador.flashTimer - dt);
            jogador.animTimer  += dt;

            if (GetRandomValue(0, 100) < 6) {
                for (int ty=0; ty<MAP_H; ty++) for (int tx=0; tx<MAP_W; tx++) {
                    if (map[ty][tx] == 2 && GetRandomValue(0,30)==0) {
                        float ang = (float)GetRandomValue(0,360)*DEG2RAD;
                        spawnParticle(tx*TS+TS/2.0f, ty*TS+TS/2.0f, cosf(ang)*12, sinf(ang)*12 - 20, 1.2f, 2.5f, C_PORTAL);
                    }
                }
            }

            if (IsKeyPressed(KEY_ESCAPE)) { pauseReturn = GameState::PLAYING; pauseSel = 0; state = GameState::PAUSE; }
            if (IsKeyPressed(KEY_I)) state = GameState::INVENTORY;
            if (IsKeyPressed(KEY_Q)) state = GameState::SKILL_MENU;
            break;

        case GameState::BATTLE:
            if (IsKeyPressed(KEY_ESCAPE)) { pauseReturn = GameState::BATTLE; pauseSel = 0; state = GameState::PAUSE; break; }
            if (battle.acabou) {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                    encerrarBatalha();
            } else {
                batalhaAcaoInimigo(dt);
                if (IsKeyPressed(KEY_ONE)   || IsKeyPressed(KEY_KP_1)) batalhaAcaoJogador(0);
                if (IsKeyPressed(KEY_TWO)   || IsKeyPressed(KEY_KP_2)) batalhaAcaoJogador(1);
                if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) batalhaAcaoJogador(2);
                if (IsKeyPressed(KEY_FOUR)  || IsKeyPressed(KEY_KP_4)) batalhaAcaoJogador(3);
                if (IsKeyPressed(KEY_F)) {
                    if (GetRandomValue(0,1)) {
                        logMsg("Fugiu!", C_TEXT_DIM);
                        state = GameState::PLAYING;
                        battle = {};
                    } else {
                        logMsg("Nao conseguiu fugir!", C_DAMAGE);
                    }
                }
            }
            break;

        case GameState::INVENTORY:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_I)) state = GameState::PLAYING;
            if (IsKeyPressed(KEY_UP))   invSel = max(0, invSel - 1);
            if (IsKeyPressed(KEY_DOWN)) invSel = min((int)inventario.size()-1, invSel + 1);
            if (IsKeyPressed(KEY_ENTER) && !inventario.empty()) {
                auto& it = inventario[invSel];
                if (it.tipo == "cura_vida") jogador.curar(it.valor);
                else if (it.tipo == "cura_mana") {
                    int ant = jogador.mp;
                    jogador.mp = min(jogador.mp + it.valor, jogador.mpMax);
                    logMsg("Recuperou " + to_string(jogador.mp-ant) + " MP!", C_HEAL);
                } else if (it.tipo == "buff_atk") {
                    jogador.atk += it.valor;
                    logMsg("ATK +" + to_string(it.valor) + "!", C_GOLD);
                } else if (it.tipo == "buff_def") {
                    jogador.def += it.valor;
                    logMsg("DEF +" + to_string(it.valor) + "!", C_GOLD);
                }
                inventario.erase(inventario.begin() + invSel);
                invSel = max(0, invSel - 1);
            }
            break;

        case GameState::SKILL_MENU:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) state = GameState::PLAYING;
            break;


        case GameState::PAUSE:
            if (IsKeyPressed(KEY_UP))   pauseSel = max(0, pauseSel - 1);
            if (IsKeyPressed(KEY_DOWN)) pauseSel = min(2, pauseSel + 1);
            if (IsKeyPressed(KEY_ESCAPE)) state = pauseReturn;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (pauseSel == 0) {
                    state = pauseReturn;
                } else if (pauseSel == 1) {
                    state = GameState::MENU;
                    battle = {};
                    gLog.clear();
                } else {
                    CloseWindow();
                }
            }
            break;

        case GameState::GAME_OVER:
        case GameState::VICTORY:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                state = GameState::MENU;
                andar = 1; ouro = 50; exp = 0; expProx = 100;
                gLog.clear();
                gParticles.clear();
            }
            break;
        }
    }

    // ── Draw helpers ───────────────────────────────────────────
    void drawBarra(int x, int y, int w, int h, int val, int maxVal, Color fg, Color bg) {
        DrawRectangleRounded({(float)x,(float)y,(float)w,(float)h}, 0.5f, 4, bg);
        int filled = (maxVal > 0) ? (val * w / maxVal) : 0;
        if (filled > 0) {
            Color fgHi = shade(fg, 1.35f);
            DrawRectangleGradientH(x, y, filled, h, fgHi, fg);
            DrawRectangleRoundedLines({(float)x,(float)y,(float)filled,(float)h}, 0.5f, 4, shade(fg,0.6f));
        }
        DrawRectangleRoundedLines({(float)x,(float)y,(float)w,(float)h}, 0.5f, 4, C_UI_BORD);
    }

    void drawTile(int tx, int ty, int type) {
        int sx = tx * TS - (int)camX;
        int sy = ty * TS - (int)camY;
        if (sx < -TS || sx > SCREEN_W || sy < -TS || sy > SCREEN_H) return;

        unsigned h = hashTile(tx, ty, mapSeed);
        float varT = (float)(h % 100) / 100.0f; // 0..1 variação

        if (type == 1) { // Parede
            Color base = colorLerp(C_WALL_LO, C_WALL_HI, varT * 0.5f + 0.25f);
            DrawRectangle(sx, sy, TS, TS, base);
            Color brickLine = shade(base, 0.55f);
            int rows = 3;
            for (int r = 0; r <= rows; r++)
                DrawLine(sx, sy + r*TS/rows, sx+TS, sy + r*TS/rows, brickLine);
            int off = ((ty % 2) == 0) ? TS/4 : -TS/4;
            for (int r = 0; r < rows; r++) {
                int yline = sy + r*TS/rows;
                int xline = sx + TS/2 + off;
                DrawLine(xline, yline, xline, yline + TS/rows, brickLine);
            }
            DrawRectangle(sx, sy, TS, 3, shade(base, 1.4f));
            DrawRectangle(sx, sy+TS-4, TS, 4, shade(base, 0.45f));
        } else { // Chão (0, ou base de 2/3)
            Color base = colorLerp(C_FLOOR_LO, C_FLOOR_HI, varT);
            DrawRectangle(sx, sy, TS, TS, base);
            unsigned h2 = hashTile(tx*3+1, ty*7+2, mapSeed);
            if (h2 % 5 == 0) {
                int px = sx + (int)((h2>>4)%TS);
                int py = sy + (int)((h2>>9)%TS);
                DrawCircle(px, py, 1.5f, shade(base, 0.6f));
            }
            DrawRectangleLines(sx, sy, TS, TS, shade(base, 0.7f));
        }
    }

    void drawSprite(float wx, float wy, Color c, bool bob = false, float flash = 0.0f, float scaleMul = 1.0f) {
        float sx = wx * TS - camX;
        float sy = wy * TS - camY;
        float bobOff = 0;
        if (bob) bobOff = sinf(globalTime * 4.0f) * 2.0f;
        sy += bobOff;

        float shadowW = TS * 0.6f * scaleMul;
        DrawEllipse((int)(sx + TS/2), (int)(sy + TS - 4), shadowW/2, shadowW/5, Color{0,0,0,110});

        float pad = (TS * (1.0f - 0.8f*scaleMul)) / 2.0f + 4;
        Rectangle body = { sx + pad, sy + pad - bobOff*0.3f, TS - pad*2, TS - pad*2 };

        Color bodyTop = shade(c, 1.25f);
        Color bodyBot = shade(c, 0.8f);
        if (flash > 0) {
            bodyTop = colorLerp(bodyTop, WHITE, flash);
            bodyBot = colorLerp(bodyBot, WHITE, flash);
        }

        DrawRectangleGradientV((int)body.x, (int)body.y, (int)body.width, (int)body.height, bodyTop, bodyBot);
        DrawRectangleRoundedLines(body, 0.25f, 6, shade(c, 0.5f));
        DrawRectangleLinesEx(body, 1, Color{255,255,255,40});

        float ew = body.width * 0.16f, eh = ew;
        float ey = body.y + body.height * 0.35f;
        DrawCircle((int)(body.x + body.width*0.32f), (int)ey, ew, WHITE);
        DrawCircle((int)(body.x + body.width*0.68f), (int)ey, ew, WHITE);
        DrawCircle((int)(body.x + body.width*0.32f), (int)ey, ew*0.5f, BLACK);
        DrawCircle((int)(body.x + body.width*0.68f), (int)ey, ew*0.5f, BLACK);
    }

    void drawVignette() {
        Color edge = {0,0,0,140};
        Color clear = {0,0,0,0};
        DrawRectangleGradientV(0, 0, SCREEN_W, 90, edge, clear);
        DrawRectangleGradientV(0, SCREEN_H-170, SCREEN_W, 90, clear, edge);
        DrawRectangleGradientH(0, 0, 90, SCREEN_H, edge, clear);
        DrawRectangleGradientH(SCREEN_W-90, 0, 90, SCREEN_H, clear, edge);
    }

    void drawUI() {
        int panelH = 96;
        int panelY = SCREEN_H - panelH;
        drawPanel({0, (float)panelY, (float)SCREEN_W, (float)panelH}, C_UI_BG2, C_UI_BG, C_UI_BORD, 0.0f);

        DrawText((classeNome + "  Nv." + to_string(jogador.nivel)).c_str(),
            16, panelY + 8, 13, C_GOLD);

        drawBarra(16,  panelY+27, 160, 13, jogador.hp, jogador.hpMax, C_HP_BAR, C_HP_BG);
        DrawText(("HP " + to_string(jogador.hp) + "/" + to_string(jogador.hpMax)).c_str(),
            182, panelY+27, 10, C_TEXT_DIM);
        drawBarra(16,  panelY+45, 160, 13, jogador.mp, jogador.mpMax, C_MP_BAR, C_MP_BG);
        DrawText(("MP " + to_string(jogador.mp) + "/" + to_string(jogador.mpMax)).c_str(),
            182, panelY+45, 10, C_TEXT_DIM);

        drawBarra(16, panelY+63, 160, 9, exp, expProx, {255,200,30,255}, {40,30,10,255});
        DrawText(("EXP " + to_string(exp) + "/" + to_string(expProx)).c_str(),
            182, panelY+63, 10, C_TEXT_DIM);

        DrawText(("Ouro: " + to_string(ouro)).c_str(), 350, panelY+10, 13, C_GOLD);
        DrawText(("Andar: " + to_string(andar) + "/7").c_str(), 350, panelY+28, 13, C_TEXT);
        DrawText("ATK:", 350, panelY+46, 11, C_TEXT_DIM);
        DrawText(to_string(jogador.atk).c_str(), 388, panelY+46, 11, C_TEXT);
        DrawText("DEF:", 350, panelY+60, 11, C_TEXT_DIM);
        DrawText(to_string(jogador.def).c_str(), 388, panelY+60, 11, C_TEXT);

        DrawText("[WASD] Mover  [I] Inventario  [Q] Habilidades", 470, panelY+10, 11, C_TEXT_DIM);
        DrawText("[ENTER/ESPACO] Interagir  [ESC] Voltar", 470, panelY+26, 11, C_TEXT_DIM);

        int logX = 470, logY = panelY - 12;
        for (int i = (int)gLog.size()-1; i >= 0; i--) {
            auto& l = gLog[i];
            float alpha = min(1.0f, l.timer);
            Color c = l.cor; c.a = (unsigned char)(alpha * 255);
            DrawText(l.texto.c_str(), logX+1, logY+1, 11, Color{0,0,0,(unsigned char)(c.a*0.6f)});
            DrawText(l.texto.c_str(), logX, logY, 11, c);
            logY -= 15;
        }
    }

    void drawMapa() {
    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            int t = map[ty][tx];

            if (t == 1) { 
                drawTile(tx, ty, 1); 
                continue; 
            }

            drawTile(tx, ty, 0);

            int sx = tx * TS - (int)camX;
            int sy = ty * TS - (int)camY;

            // ================= PORTAL =================
            if (t == 2) {
                float pulse = 0.65f + 0.35f * sinf(globalTime * 3.0f);

                for (int ring = 3; ring >= 1; ring--) {
                    float rad = (TS*0.18f) * ring * (0.8f + 0.2f*pulse);
                    Color rc = shade(C_PORTAL, 0.5f + 0.2f*ring);
                    rc.a = (unsigned char)(70 + 30*pulse);
                    DrawCircleLines(sx+TS/2, sy+TS/2, rad, rc);
                }

                // ✅ GRADIENTE CORRETO
                DrawCircleGradient(
                    Vector2{sx + TS/2.0f, sy + TS/2.0f},
                    TS * 0.32f * pulse,
                    Color{C_PORTAL.r, C_PORTAL.g, C_PORTAL.b, 220},
                    Color{C_PORTAL.r, C_PORTAL.g, C_PORTAL.b, 0}
                );

                DrawText("SAIDA", sx+TS/2-16, sy+TS/2-6, 10, WHITE);
            }

            // ================= ITEM =================
            else if (t == 3) {
                float bobv = sinf(globalTime * 5.0f) * 3.0f;
                float glow = 0.6f + 0.4f*sinf(globalTime*4.0f);

                // brilho azul
                DrawCircleGradient(
                    Vector2{sx + TS/2.0f, sy + TS/2.0f + bobv},
                    TS * 0.5f * glow,
                    SKYBLUE,
                    BLANK
                );

                // brilho dourado
                DrawCircleGradient(
                    Vector2{sx + TS/2.0f, sy + TS/2.0f + bobv},
                    TS * 0.32f,
                    Color{255,230,120,90},
                    Color{255,230,120,0}
                );

                Vector2 c = {(float)(sx+TS/2), (float)(sy+TS/2+bobv)};

                Vector2 pts[4] = {
                    {c.x, c.y-9}, {c.x+9, c.y}, {c.x, c.y+9}, {c.x-9, c.y}
                };

                DrawTriangle(pts[0], pts[1], c, C_ITEM);
                DrawTriangle(pts[1], pts[2], c, shade(C_ITEM,0.85f));
                DrawTriangle(pts[2], pts[3], c, shade(C_ITEM,0.7f));
                DrawTriangle(pts[3], pts[0], c, shade(C_ITEM,0.85f));

                DrawCircleLines((int)c.x, (int)c.y, 10, WHITE);
            }
        }
    }

        for (auto& p : gParticles) {
            int sx = (int)(p.x - camX);
            int sy = (int)(p.y - camY);
            float a = Clamp(p.life / p.maxLife, 0.0f, 1.0f);
            Color c = p.cor; c.a = (unsigned char)(a*255);
            DrawCircle(sx, sy, p.size, c);
        }
    }

    void drawInimigos() {
        for (auto& e : inimigos) {
            if (!e.vivo) continue;
            float shk = (e.shakeTimer > 0) ? e.shakeDir * 2.0f : 0;
            float flashAmt = (e.flashTimer > 0) ? (e.flashTimer / 0.15f) : 0.0f;
            float scaleMul = (e.nome == "Dragao") ? 1.15f : 1.0f;
            drawSprite(e.x + shk/TS, e.y, e.cor, true, flashAmt, scaleMul);
            int sx = (int)(e.x * TS - camX);
            int sy = (int)(e.y * TS - camY);
            drawBarra(sx+2, sy-10, TS-4, 6, e.hp, e.hpMax, C_DAMAGE, C_HP_BG);
        }
    }

    void drawBattle() {
        if (!battle.alvo) return;
        Inimigo* e = battle.alvo;

        DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, Color{4,3,10,190}, Color{16,8,28,235});

        int bx = SCREEN_W/2 - 390, by = SCREEN_H/2 - 255;
        int bw = 780, bh = 510;
        drawPanel({(float)bx,(float)by,(float)bw,(float)bh}, Color{22,18,42,245}, Color{8,7,18,245}, C_UI_BORD, 0.04f);

        DrawText("BATALHA", bx + bw/2 - 44, by + 14, 18, C_GOLD);
        DrawLine(bx+22, by+42, bx+bw-22, by+42, Color{120,90,170,130});

        Rectangle arena = {(float)(bx+28), (float)(by+58), (float)(bw-56), 230};
        DrawRectangleRounded(arena, 0.04f, 8, Color{12,10,24,220});
        DrawRectangleRoundedLines(arena, 0.04f, 8, Color{120,90,170,90});
        DrawLine((int)arena.x+20, (int)(arena.y+arena.height-34), (int)(arena.x+arena.width-20), (int)(arena.y+arena.height-34), Color{120,90,170,60});

        float pulse = 0.6f + 0.4f*sinf(globalTime*3.0f);
        DrawCircleGradient(Vector2{arena.x + arena.width/2, arena.y + 112}, 120.0f, Color{90,55,150,(unsigned char)(55 + 25*pulse)}, Color{0,0,0,0});
        DrawText("VS", bx + bw/2 - 18, by + 148, 28, Color{170,150,230,210});

        int px = bx + 110, py = by + 105;
        int ex = bx + bw - 230, ey = by + 90;
        float flashP = jogador.flashTimer > 0 ? (jogador.flashTimer/0.15f) : 0.0f;
        float flashE = e->flashTimer > 0 ? (e->flashTimer/0.15f) : 0.0f;

        DrawEllipse(px+58, py+126, 72, 15, Color{0,0,0,140});
        Rectangle pbody = {(float)px, (float)py, 116, 116};
        DrawRectangleGradientV(px, py, 116, 116, colorLerp(shade(jogador.cor,1.3f), WHITE, flashP), colorLerp(shade(jogador.cor,0.75f), WHITE, flashP));
        DrawRectangleRoundedLines(pbody, 0.18f, 8, WHITE);
        DrawCircle(px+38, py+45, 10, WHITE);
        DrawCircle(px+78, py+45, 10, WHITE);
        DrawCircle(px+38, py+45, 5, BLACK);
        DrawCircle(px+78, py+45, 5, BLACK);

        DrawEllipse(ex+58, ey+141, 76, 16, Color{0,0,0,140});
        Rectangle ebody = {(float)ex, (float)ey, 116, 132};
        DrawRectangleGradientV(ex, ey, 116, 132, colorLerp(shade(e->cor,1.3f), WHITE, flashE), colorLerp(shade(e->cor,0.75f), WHITE, flashE));
        DrawRectangleRoundedLines(ebody, 0.18f, 8, WHITE);
        DrawCircle(ex+38, ey+52, 10, WHITE);
        DrawCircle(ex+78, ey+52, 10, WHITE);
        DrawCircle(ex+38, ey+52, 5, BLACK);
        DrawCircle(ex+78, ey+52, 5, BLACK);

        Rectangle pstats = {(float)(bx+32), (float)(by+304), 250, 92};
        Rectangle estats = {(float)(bx+bw-282), (float)(by+304), 250, 92};
        DrawRectangleRounded(pstats, 0.06f, 8, Color{18,15,34,230});
        DrawRectangleRounded(estats, 0.06f, 8, Color{18,15,34,230});
        DrawRectangleRoundedLines(pstats, 0.06f, 8, Color{120,90,170,80});
        DrawRectangleRoundedLines(estats, 0.06f, 8, Color{120,90,170,80});

        DrawText(classeNome.c_str(), bx+48, by+318, 14, C_GOLD);
        DrawText(("Nv." + to_string(jogador.nivel) + "  ATK " + to_string(jogador.atk) + "  DEF " + to_string(jogador.def)).c_str(), bx+48, by+336, 10, C_TEXT_DIM);
        drawBarra(bx+48, by+354, 190, 12, jogador.hp, jogador.hpMax, C_HP_BAR, C_HP_BG);
        drawBarra(bx+48, by+373, 190, 12, jogador.mp, jogador.mpMax, C_MP_BAR, C_MP_BG);
        DrawText(("HP " + to_string(jogador.hp) + "/" + to_string(jogador.hpMax)).c_str(), bx+244, by+353, 10, C_TEXT_DIM);
        DrawText(("MP " + to_string(jogador.mp) + "/" + to_string(jogador.mpMax)).c_str(), bx+244, by+372, 10, C_TEXT_DIM);

        DrawText(e->nome.c_str(), bx+bw-266, by+318, 14, C_DAMAGE);
        DrawText(("Nv." + to_string(e->nivel) + "  ATK " + to_string(e->atk) + "  DEF " + to_string(e->def)).c_str(), bx+bw-266, by+336, 10, C_TEXT_DIM);
        drawBarra(bx+bw-266, by+354, 190, 12, e->hp, e->hpMax, C_HP_BAR, C_HP_BG);
        DrawText(("HP " + to_string(e->hp) + "/" + to_string(e->hpMax)).c_str(), bx+bw-70, by+353, 10, C_TEXT_DIM);

        Rectangle turnBox = {(float)(bx+302), (float)(by+312), 176, 52};
        Color turnColor = battle.jogadorVez ? C_HEAL : C_DAMAGE;
        DrawRectangleRounded(turnBox, 0.08f, 8, Color{12,10,24,235});
        DrawRectangleRoundedLines(turnBox, 0.08f, 8, turnColor);
        const char* vez = battle.jogadorVez ? "SUA VEZ" : "INIMIGO";
        DrawText(vez, bx+356, by+324, 14, turnColor);

        Rectangle cmd = {(float)(bx+32), (float)(by+410), 326, 80};
        Rectangle logBox = {(float)(bx+378), (float)(by+410), 370, 80};
        DrawRectangleRounded(cmd, 0.05f, 8, Color{12,10,24,235});
        DrawRectangleRounded(logBox, 0.05f, 8, Color{12,10,24,235});
        DrawRectangleRoundedLines(cmd, 0.05f, 8, Color{120,90,170,85});
        DrawRectangleRoundedLines(logBox, 0.05f, 8, Color{120,90,170,85});

        if (battle.acabou) {
            const char* msg = battle.vitoria ? "VITORIA!  [ENTER]" : "DERROTA!";
            Color mc = battle.vitoria ? C_GOLD : C_DAMAGE;
            DrawText(msg, bx+94, by+438, 18, mc);
        } else if (battle.jogadorVez) {
            DrawText("[1] Atacar", bx+50, by+426, 12, C_TEXT);
            for (int i = 0; i < (int)habilidades.size(); i++) {
                string hab = "[" + to_string(i+2) + "] " + habilidades[i].nome + "  " + to_string(habilidades[i].custoMp) + " MP";
                Color hc = (jogador.mp >= habilidades[i].custoMp) ? C_TEXT : C_TEXT_DIM;
                DrawText(hab.c_str(), bx+50, by+444 + 15*i, 11, hc);
            }
            DrawText("[F] Fugir", bx+218, by+426, 12, C_TEXT_DIM);
        } else {
            DrawText("Aguarde...", bx+50, by+444, 13, C_TEXT_DIM);
        }

        int ly = by + 424;
        int firstLog = max(0, (int)gLog.size() - 4);
        for (int i = firstLog; i < (int)gLog.size(); i++) {
            DrawText(gLog[i].texto.c_str(), bx+396, ly, 10, gLog[i].cor);
            ly += 15;
        }

        for (auto& f : battle.floatTexts) {
            float alpha = min(1.0f, f.timer);
            Color fc = f.cor; fc.a = (unsigned char)(alpha * 255);
            DrawText(f.txt.c_str(), (int)f.x+1, (int)f.y+1, 18, Color{0,0,0,(unsigned char)(fc.a*0.6f)});
            DrawText(f.txt.c_str(), (int)f.x, (int)f.y, 18, fc);
        }
    }

    void drawInventory() {
        DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H, Color{0,0,0,150}, Color{5,5,15,210});
        int px = SCREEN_W/2 - 210, py = SCREEN_H/2 - 190;
        drawPanel({(float)px,(float)py,420,380}, C_UI_BG2, C_UI_BG, C_UI_BORD, 0.05f);
        DrawText("INVENTARIO", px + 150, py + 14, 15, C_GOLD);
        DrawText("[ESC] Fechar  [ENTER] Usar", px + 90, py + 34, 10, C_TEXT_DIM);
        DrawLine(px+20, py+50, px+400, py+50, Color{120,90,170,120});

        if (inventario.empty()) {
            DrawText("(vazio)", px + 170, py + 110, 12, C_TEXT_DIM);
        } else {
            for (int i = 0; i < (int)inventario.size(); i++) {
                bool sel = (i == invSel);
                if (sel) DrawRectangleRounded({(float)(px+12), (float)(py+62+i*24-3), 396, 22}, 0.3f, 4, Color{120,90,170,60});
                Color c = sel ? C_GOLD : C_TEXT;
                string prefix = sel ? "> " : "  ";
                DrawText((prefix + inventario[i].nome).c_str(), px + 24, py + 64 + i*24, 12, c);
            }
        }
    }

    void drawSkillMenu() {
        DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H, Color{0,0,0,150}, Color{5,5,15,210});
        int px = SCREEN_W/2 - 230, py = SCREEN_H/2 - 190;
        drawPanel({(float)px,(float)py,460,380}, C_UI_BG2, C_UI_BG, C_UI_BORD, 0.05f);
        DrawText("HABILIDADES", px + 160, py + 14, 15, C_GOLD);
        DrawText("[Q] Fechar", px + 190, py + 34, 10, C_TEXT_DIM);
        DrawLine(px+20, py+50, px+440, py+50, Color{120,90,170,120});

        for (int i = 0; i < (int)habilidades.size(); i++) {
            auto& h = habilidades[i];
            int hy = py + 64 + i * 54;
            bool ok = jogador.mp >= h.custoMp;
            DrawRectangleRounded({(float)(px+14), (float)hy-4, 432, 48}, 0.15f, 4, ok ? Color{80,60,120,50} : Color{40,30,50,40});
            Color c = ok ? C_TEXT : C_TEXT_DIM;
            DrawText(("[" + to_string(i+2) + "] " + h.nome).c_str(), px+26, hy,    13, c);
            DrawText(h.desc.c_str(), px+26, hy+16, 10, C_TEXT_DIM);
            DrawText(("MP: " + to_string(h.custoMp) +
                      "  Dano: " + to_string(h.multPct) + "% ATK").c_str(),
                px+26, hy+30, 10, ok ? C_MP_BAR : C_DAMAGE);
        }
    }

    void drawPauseMenu() {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Color{0,0,0,150});

        int pw = 420, ph = 260;
        int px = SCREEN_W/2 - pw/2;
        int py = SCREEN_H/2 - ph/2;
        drawPanel({(float)px, (float)py, (float)pw, (float)ph}, Color{31,15,20,248}, Color{10,7,18,248}, C_UI_BORD, 0.06f);

        DrawText("PAUSADO", px + pw/2 - MeasureText("PAUSADO", 24)/2, py + 26, 24, C_GOLD);
        DrawLine(px+34, py+62, px+pw-34, py+62, Color{170,70,20,150});

        const char* opts[] = {"CONTINUAR", "MENU PRINCIPAL", "SAIR"};
        for (int i = 0; i < 3; i++) {
            bool sel = (i == pauseSel);
            Rectangle r = {(float)(px+56), (float)(py+88+i*48), (float)(pw-112), 36};
            if (sel) DrawRectangleRounded(r, 0.20f, 8, Color{120,20,10,220});
            DrawRectangleRoundedLines(r, 0.20f, 8, sel ? C_GOLD : Color{100,40,10,180});
            string label = sel ? "> " + string(opts[i]) : "  " + string(opts[i]);
            int tw = MeasureText(label.c_str(), 18);
            DrawText(label.c_str(), (int)(r.x + r.width/2 - tw/2), (int)(r.y + 9), 18, sel ? C_GOLD : C_TEXT_DIM);
        }

        DrawText("[ESC] Voltar   [ENTER] Confirmar", px + pw/2 - 139, py + ph - 32, 13, C_TEXT_DIM);
    }

    void drawMenu() {
        for (int y = 0; y < SCREEN_H; y++) {
            float t = (float)y / SCREEN_H;
            unsigned char r = (unsigned char)(8  + t * 30);
            unsigned char g = (unsigned char)(5  + t * 10);
            unsigned char b = (unsigned char)(18 + t * 20);
            DrawLine(0, y, SCREEN_W, y, Color{r, g, b, 255});
        }

        for (int i = 0; i < 180; i++) {
            unsigned h = hashTile(i, 19, 77);
            int sx = h % SCREEN_W;
            int sy = (h / 11) % (int)(SCREEN_H * 0.65f);
            float brilho = 0.35f + 0.65f * sinf(globalTime * (0.5f + (i % 9) * 0.18f) + i);
            DrawPixel(sx, sy, Color{255, 240, 200, (unsigned char)(80 + 130 * brilho)});
        }

        Vector2 lua = {SCREEN_W * 0.76f, SCREEN_H * 0.18f};
        float raioLua = SCREEN_H * 0.095f;
        DrawCircleV(lua, raioLua + 30, Color{220, 200, 150, 15});
        DrawCircleV(lua, raioLua + 18, Color{220, 200, 150, 30});
        DrawCircleV(lua, raioLua + 8, Color{220, 200, 150, 50});
        DrawCircleV(lua, raioLua, Color{240, 225, 180, 255});
        DrawCircleV(Vector2{lua.x + 22, lua.y - 8}, raioLua * 0.85f, Color{12, 8, 22, 200});

        Vector2 montanha1[] = {
            {0, (float)SCREEN_H}, {0, SCREEN_H*0.70f}, {SCREEN_W*0.06f, SCREEN_H*0.64f},
            {SCREEN_W*0.13f, SCREEN_H*0.71f}, {SCREEN_W*0.20f, SCREEN_H*0.60f}, {SCREEN_W*0.28f, SCREEN_H*0.54f},
            {SCREEN_W*0.36f, SCREEN_H*0.63f}, {SCREEN_W*0.43f, SCREEN_H*0.51f}, {SCREEN_W*0.50f, SCREEN_H*0.58f},
            {SCREEN_W*0.57f, SCREEN_H*0.50f}, {SCREEN_W*0.64f, SCREEN_H*0.61f}, {SCREEN_W*0.72f, SCREEN_H*0.53f},
            {SCREEN_W*0.80f, SCREEN_H*0.63f}, {SCREEN_W*0.88f, SCREEN_H*0.56f}, {SCREEN_W*0.96f, SCREEN_H*0.67f},
            {(float)SCREEN_W, SCREEN_H*0.68f}, {(float)SCREEN_W, (float)SCREEN_H}
        };
        DrawTriangleFan(montanha1, 17, Color{18, 12, 35, 255});

        Vector2 montanha2[] = {
            {0, (float)SCREEN_H}, {0, SCREEN_H*0.78f}, {SCREEN_W*0.08f, SCREEN_H*0.74f},
            {SCREEN_W*0.16f, SCREEN_H*0.79f}, {SCREEN_W*0.25f, SCREEN_H*0.71f}, {SCREEN_W*0.34f, SCREEN_H*0.75f},
            {SCREEN_W*0.44f, SCREEN_H*0.70f}, {SCREEN_W*0.54f, SCREEN_H*0.76f}, {SCREEN_W*0.64f, SCREEN_H*0.70f},
            {SCREEN_W*0.74f, SCREEN_H*0.75f}, {SCREEN_W*0.84f, SCREEN_H*0.71f}, {SCREEN_W*0.93f, SCREEN_H*0.76f},
            {(float)SCREEN_W, SCREEN_H*0.74f}, {(float)SCREEN_W, (float)SCREEN_H}
        };
        DrawTriangleFan(montanha2, 14, Color{8, 5, 15, 255});

        for (int i = 0; i < 6; i++) {
            float yFog = SCREEN_H * 0.68f + i * 12.0f;
            DrawRectangle(0, (int)yFog, SCREEN_W, 15, Color{60, 30, 80, (unsigned char)(30 - i * 4)});
        }

        for (int i = 0; i < 120; i++) {
            unsigned h = hashTile(i, 31, 123);
            float baseX = (float)(h % SCREEN_W);
            float baseY = SCREEN_H * 0.56f + (float)((h / 7) % (int)(SCREEN_H * 0.40f));
            float life = fmodf(globalTime * (0.18f + (i % 5) * 0.04f) + (h % 100) / 100.0f, 1.0f);
            float x = baseX + sinf(globalTime + i) * 18.0f;
            float y = baseY - life * 170.0f;
            float size = (2.0f + (h % 4)) * (1.0f - life);
            DrawCircleV(Vector2{x, y}, size, Color{(unsigned char)(180 + h % 70), (unsigned char)(60 + h % 80), 20, (unsigned char)(180 * (1.0f - life))});
        }

        const char* titulo1 = "ARCANO DAS";
        const char* titulo2 = "SOMBRAS";
        int font1 = 72;
        int font2 = 94;
        int t1w = MeasureText(titulo1, font1);
        int t2w = MeasureText(titulo2, font2);
        float pulso = 0.5f + 0.5f * sinf(globalTime * 1.8f);
        int titleY = (int)(SCREEN_H * 0.18f);

        DrawRectangle(SCREEN_W/2 - 340, titleY - 8, 680, 150, Color{80, 20, 5, (unsigned char)(18 * pulso)});
        DrawText(titulo1, SCREEN_W/2 - t1w/2 + 4, titleY + 4, font1, Color{0,0,0,120});
        DrawText(titulo2, SCREEN_W/2 - t2w/2 + 4, titleY + 72, font2, Color{0,0,0,120});

        Color corTitulo1 = colorLerp(Color{150, 55, 18, 255}, Color{235, 105, 30, 255}, pulso);
        Color corTitulo2 = colorLerp(Color{180, 75, 20, 255}, Color{255, 145, 38, 255}, pulso);
        DrawText(titulo1, SCREEN_W/2 - t1w/2, titleY, font1, corTitulo1);
        DrawText(titulo2, SCREEN_W/2 - t2w/2, titleY + 68, font2, corTitulo2);

        int lx = SCREEN_W/2;
        int lineY = titleY + 190;
        DrawLine(lx - 220, lineY, lx + 220, lineY, Color{160, 60, 10, 180});
        DrawLine(lx - 220, lineY + 2, lx + 220, lineY + 2, Color{80, 30, 5, 100});

        const char* sub = "Uma aventura nas terras esquecidas";
        int sw = MeasureText(sub, 20);
        DrawText(sub, SCREEN_W/2 - sw/2, lineY + 16, 20, Color{160, 120, 80, 180});

        const char* opts[] = {"INICIAR JORNADA", "SAIR"};
        int buttonW = 320;
        int buttonH = 58;
        int buttonX = SCREEN_W/2 - buttonW/2;
        int buttonY = lineY + 62;
        for (int i = 0; i < 2; i++) {
            bool sel = (i == menuSel);
            float escala = sel ? 1.04f : 1.0f;
            float w = buttonW * escala;
            float h = buttonH * escala;
            Rectangle r = {buttonX - (w-buttonW)/2, (float)(buttonY + i*78) - (h-buttonH)/2, w, h};
            Color fundo = sel ? Color{120, 20, 10, 220} : Color{40, 10, 5, 200};
            Color borda = sel ? colorLerp(Color{150, 55, 15, 255}, Color{255, 110, 30, 255}, pulso) : Color{100, 40, 10, 255};
            DrawRectangleRounded(r, 0.2f, 8, fundo);
            DrawRectangleRoundedLines(r, 0.2f, 8, borda);
            string label = sel ? "> " + string(opts[i]) : "  " + string(opts[i]);
            int fs = sel ? 25 : 23;
            int tw = MeasureText(label.c_str(), fs);
            DrawText(label.c_str(), (int)(r.x + r.width/2 - tw/2), (int)(r.y + r.height/2 - fs/2), fs, sel ? Color{255, 200, 80, 255} : Color{200, 140, 60, 255});
        }

        DrawText("[SETAS] Navegar   [ENTER] Confirmar", SCREEN_W/2 - 170, SCREEN_H - 34, 15, Color{130, 100, 75, 180});
        DrawText("v0.1 - Alpha", 12, SCREEN_H - 26, 16, Color{80, 60, 40, 160});
    }

    void drawCharSelect() {
        DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H, Color{20,16,36,255}, C_BG);
        DrawText("ESCOLHA SUA CLASSE", SCREEN_W/2 - 112, 40, 18, C_GOLD);
        DrawText("[SETA ESQ/DIR] Navegar  [ENTER] Confirmar  [ESC] Voltar",
            SCREEN_W/2 - 200, SCREEN_H - 35, 11, C_TEXT_DIM);

        struct ClassInfo { const char* nome; const char* desc[4]; Color cor; };
        ClassInfo classes[] = {
            {"GUERREIRO", {"HP: 160  MP: 40", "ATK: 20  DEF: 14", "Tanque fisico robusto", "Alta vida e defesa"}, C_PLAYER},
            {"MAGO",      {"HP: 90   MP: 130","ATK: 32  DEF: 4",  "Dano magico explosivo", "Fraco fisicamente"}, {180,100,255,255}},
            {"LADINO",    {"HP: 110  MP: 70", "ATK: 24  DEF: 6",  "Evasao e criticos", "28% de chance de desviar"}, {100,255,160,255}},
        };

        for (int i = 0; i < 3; i++) {
            int cx = 130 + i * 200, cy = SCREEN_H/2 - 140;
            bool sel = (i == classSel);
            Color border = sel ? C_GOLD : C_UI_BORD;
            drawPanel({(float)(cx - 85), (float)cy, 170, 290}, C_UI_BG2, C_UI_BG, border, 0.08f);

            int sy = cy + 30;
            float bob = sel ? sinf(globalTime*3.0f)*3.0f : 0;
            DrawEllipse(cx, sy+62, 30, 7, Color{0,0,0,100});
            Rectangle sprBody = {(float)(cx-25), sy+(float)bob, 50, 50};
            DrawRectangleGradientV((int)sprBody.x,(int)sprBody.y,50,50, shade(classes[i].cor,1.25f), shade(classes[i].cor,0.8f));
            DrawRectangleRoundedLines(sprBody, 0.2f, 6, WHITE);

            DrawText(classes[i].nome, cx - (int)(strlen(classes[i].nome)*4), cy + 14, 12,
                sel ? C_GOLD : C_TEXT);
            for (int j = 0; j < 4; j++)
                DrawText(classes[i].desc[j], cx - 65, cy + 105 + j*22, 10,
                    sel ? C_TEXT : C_TEXT_DIM);

            if (sel) DrawText("[ SELECIONADO ]", cx - 55, cy + 258, 11, C_GOLD);
        }
    }

    void drawGameOver() {
        DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H, Color{30,4,4,255}, Color{8,0,0,255});
        float pulse = 1.0f + 0.04f*sinf(globalTime*2.5f);
        DrawText("GAME OVER", (int)(SCREEN_W/2 - 84*pulse), SCREEN_H/2 - 60, (int)(34*pulse), C_DAMAGE);
        DrawText(("Chegou ao Andar " + to_string(andar)).c_str(),
            SCREEN_W/2 - 80, SCREEN_H/2, 16, C_TEXT_DIM);
        DrawText("[ENTER] Menu Principal", SCREEN_W/2 - 90, SCREEN_H/2 + 50, 14, C_TEXT);
    }

    void drawVictory() {
        DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H, Color{4,30,8,255}, Color{0,8,2,255});
        for (int i = 0; i < 40; i++) {
            unsigned h = hashTile(i, 13, 55);
            int sx = h % SCREEN_W;
            int sy = (int)((h/7) % SCREEN_H + globalTime*40) % SCREEN_H;
            DrawCircle(sx, sy, 1.5f, Color{255,220,100,150});
        }
        float pulse = 1.0f + 0.05f*sinf(globalTime*2.5f);
        DrawText("VITORIA!", (int)(SCREEN_W/2 - 74*pulse), SCREEN_H/2 - 80, (int)(34*pulse), C_GOLD);
        DrawText("Voce conquistou a masmorra!", SCREEN_W/2 - 130, SCREEN_H/2 - 30, 16, C_HEAL);
        DrawText(("Nivel final: " + to_string(jogador.nivel)).c_str(),
            SCREEN_W/2 - 70, SCREEN_H/2 + 10, 14, C_TEXT);
        DrawText(("Ouro total: " + to_string(ouro)).c_str(),
            SCREEN_W/2 - 70, SCREEN_H/2 + 30, 14, C_GOLD);
        DrawText("[ENTER] Menu Principal", SCREEN_W/2 - 90, SCREEN_H/2 + 80, 14, C_TEXT);
    }

    void draw() {
        BeginDrawing();
        ClearBackground(C_BG);

        switch (state) {
        case GameState::MENU:
            drawMenu(); break;
        case GameState::CHAR_SELECT:
            drawCharSelect(); break;
        case GameState::GAME_OVER:
            drawGameOver(); break;
        case GameState::VICTORY:
            drawVictory(); break;

        case GameState::PLAYING:
        case GameState::BATTLE:
        case GameState::INVENTORY:
        case GameState::SKILL_MENU:
        case GameState::PAUSE:
            drawMapa();
            drawInimigos();
            {
                float shk = (jogador.shakeTimer > 0) ? jogador.shakeDir * 2.0f : 0;
                float flashAmt = jogador.flashTimer > 0 ? (jogador.flashTimer/0.15f) : 0;
                drawSprite(jogador.x + shk/TS, jogador.y, jogador.cor, false, flashAmt);
            }
            drawVignette();
            drawUI();

            if (state == GameState::BATTLE || (state == GameState::PAUSE && pauseReturn == GameState::BATTLE)) drawBattle();
            if (state == GameState::INVENTORY) drawInventory();
            if (state == GameState::SKILL_MENU)drawSkillMenu();
            if (state == GameState::PAUSE)     drawPauseMenu();
            break;
        }

        if (flashTimer > 0) {
            Color fc = flashColor;
            fc.a = (unsigned char)(flashTimer * 255);
            DrawRectangle(0, 0, SCREEN_W, SCREEN_H, fc);
        }

        DrawFPS(SCREEN_W - 60, 5);

        EndDrawing();
    }

    void run() {
        InitWindow(SCREEN_W, SCREEN_H, "Dungeon RPG — Raylib");
        SetExitKey(KEY_NULL);
        SetTargetFPS(60);
        init();
        while (!WindowShouldClose()) {
            update(GetFrameTime());
            draw();
        }
        CloseWindow();
    }
};

int main() {
    Game g;
    g.run();
    return 0;
}
