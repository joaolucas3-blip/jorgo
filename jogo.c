/* ==========================================================
   CASA VERDE: A CACADA
   Jogo inspirado em "O Alienista" (Machado de Assis)
   INFO 1V - Introducao a Programacao - IFRN Canguaretama

   COMO COMPILAR NO DEV-C++:
   1) Arquivo > Novo > Arquivo Fonte, colar este codigo e
      salvar como .c (exemplo: jogo.c)
   2) Ferramentas > Opcoes do Compilador > marcar "Adicionar os
      comandos abaixo a linha de comando do linker" e escrever:
      -mwindows -lgdi32
   3) Executar > Compilar e Executar (F11)
   ========================================================== */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

/* ---------------- CONSTANTES ---------------- */
#define LARGURA 960
#define ALTURA 640
#define MAPA_W 2400
#define MAPA_H 1800
#define PI 3.14159265f
#define DOIS_PI 6.2831853f

#define MAX_OBST 400
#define MAX_PREDIOS 40
#define MAX_INIM 120
#define MAX_PROJ 160
#define MAX_PART 700
#define MAX_MANCHAS 350

#define ESPESSURA 14
#define LARG_PORTA 64

#define JOG_RAIO 10
#define CARRO_RAIO 18
#define JOG_VEL 3.1f
#define HP_MAX 100
#define VIDAS_INICIAIS 2

#define ALMA_RAIO 170
#define ALMA_DURACAO 45
#define ALMA_RECARGA 480
#define PARRY_JANELA 14
#define PARRY_RECARGA 45
#define INVIS_DURACAO 150
#define INVIS_RECARGA 480
#define COMBO_TEMPO 360
#define XP_POR_KILL 6

#define ESTADO_MENU 0
#define ESTADO_JOGANDO 1
#define ESTADO_PAUSA 2
#define ESTADO_GAMEOVER 3
#define ESTADO_VITORIA 4

/* ---------------- ESTRUTURAS ---------------- */

/* Parede, poste, lixeira ou placa */
typedef struct {
    float x, y, w, h;
    int tipo;      /* 0=parede 1=poste madeira 2=poste ferro 3=lixeira 4=placa */
    int vivo;
    int hp;
    COLORREF cor;
} Obstaculo;

/* Piso de uma construcao (so para desenhar) */
typedef struct {
    float x, y, w, h;
    int estilo;    /* 0=japonesa 1=europeia 2=hospital 3=casa verde */
} Predio;

typedef struct {
    float x, y, ang, giro, vel_voo;
    int tipo;      /* 0=zumbi 1=vampiro 2=lobisomem 3=demonio */
    int hp, vivo, louco;
    int estado;    /* 0=normal 1=ajoelhado 2=lancado (atropelado) */
    int timer, cd_ataque, cd_tiro, dir_timer, dash;
} Inimigo;

typedef struct {
    float x, y, vx, vy;
    int vida, dono, dano, ativo; /* dono: 0=jogador 1=inimigo 2=refletido */
} Projetil;

typedef struct {
    float x, y, vx, vy, raio;
    int vida, vida_max, tipo, ativo; /* 0=sangue 1=fumaca 2=alma 3=faisca */
} Particula;

typedef struct {
    float x, y;
    int r;
} Mancha;

typedef struct {
    float x, y, mira;
    int hp, vidas, cartuchos, recarga, cd_tiro;
    int invisivel, cd_invis, parry, cd_parry, cd_alma, sugando, invul, no_carro;
} Jogador;

typedef struct {
    float x, y, ang, vel;
} Carro;

/* ---------------- VARIAVEIS GLOBAIS ---------------- */
static Obstaculo obst[MAX_OBST];
static int n_obst = 0;
static Predio predios[MAX_PREDIOS];
static int n_predios = 0;
static Inimigo inim[MAX_INIM];
static Projetil proj[MAX_PROJ];
static Particula part[MAX_PART];
static int idx_part = 0;
static Mancha manchas[MAX_MANCHAS];
static int n_manchas = 0, idx_mancha = 0;
static Jogador jog;
static Carro carro;

static int estado = ESTADO_MENU;
static int selecao = 0, dificuldade = 0;
static int xp = 0, kills = 0, combo = 0, maior_combo = 0, combo_timer = 0;
static int loucos_total = 0, loucos_capt = 0, tempo_restante = 0;
static int tremor = 0, msg_timer = 0, frame = 0, spawn_timer = 0;
static char mensagem[80] = "";
static char motivo[80] = "";

static int teclas[256];
static int mouse_x = 0, mouse_y = 0, mouse_botao = 0;
static float cam_x = 0, cam_y = 0;
static int off_x = 0, off_y = 0;

static HFONT f_titulo, f_grande, f_media, f_peq;
static HDC hdc_buffer;
static HBITMAP bmp_buffer, bmp_antigo;

/* Tabelas de dificuldade */
static const char *nome_dif[4] = {"FACIL", "MEDIO", "DIFICIL", "NIGHTMARE"};
static const int extra_loucos[4] = {0, 4, 7, 20};
static const int tempo_dif[4] = {240, 270, 300, 360};
static const int cap_comuns[4] = {10, 14, 20, 34};

/* Tabelas dos inimigos: zumbi, vampiro, lobisomem, demonio */
static const int hp_tipo[4] = {3, 5, 12, 18};
static const float vel_tipo[4] = {1.0f, 1.7f, 2.3f, 1.4f};
static const float raio_tipo[4] = {11, 11, 14, 13};
static const int dano_tipo[4] = {10, 12, 20, 12};

/* ---------------- FUNCOES UTILITARIAS ---------------- */

/* Numero real aleatorio entre a e b */
static float aleat(float a, float b)
{
    return a + (b - a) * ((float)rand() / (float)RAND_MAX);
}

/* Numero inteiro aleatorio entre a e b */
static int aleat_int(int a, int b)
{
    return a + rand() % (b - a + 1);
}

/* Distancia entre dois pontos */
static float distancia(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1, dy = y2 - y1;
    return (float)sqrt(dx * dx + dy * dy);
}

/* Mantem um valor dentro de um intervalo */
static float limitar(float v, float a, float b)
{
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

/* Multiplicador de XP do atropelamento conforme a velocidade */
static int mult_vel(float v)
{
    if (v <= 50) return 1;
    if (v < 100) return 3;
    return 10;
}

/* ---------------- COLISAO ---------------- */

/* Testa se um circulo encosta em um retangulo */
static int circ_ret(float cx, float cy, float r, float rx, float ry, float rw, float rh)
{
    float px = limitar(cx, rx, rx + rw);
    float py = limitar(cy, ry, ry + rh);
    float dx = cx - px, dy = cy - py;
    return (dx * dx + dy * dy) < r * r;
}

/* Retorna o indice do obstaculo que encosta no circulo, ou -1 */
static int obstaculo_em(float x, float y, float r)
{
    int i;
    for (i = 0; i < n_obst; i++) {
        if (!obst[i].vivo) continue;
        if (circ_ret(x, y, r, obst[i].x, obst[i].y, obst[i].w, obst[i].h)) return i;
    }
    return -1;
}

/* Move um circulo deslizando nas paredes (testa X e Y separados) */
static void mover_com_colisao(float *x, float *y, float dx, float dy, float r)
{
    if (obstaculo_em(*x + dx, *y, r) < 0) *x += dx;
    if (obstaculo_em(*x, *y + dy, r) < 0) *y += dy;
}

/* ---------------- EFEITOS (PARTICULAS) ---------------- */

static void criar_particula(float x, float y, float vx, float vy, float raio, int vida, int tipo)
{
    Particula *p = &part[idx_part];
    idx_part = (idx_part + 1) % MAX_PART;
    p->x = x; p->y = y; p->vx = vx; p->vy = vy;
    p->raio = raio; p->vida = vida; p->vida_max = vida;
    p->tipo = tipo; p->ativo = 1;
}

/* Mancha de sangue fixa no chao */
static void mancha_chao(float x, float y, int r)
{
    manchas[idx_mancha].x = x;
    manchas[idx_mancha].y = y;
    manchas[idx_mancha].r = r;
    idx_mancha = (idx_mancha + 1) % MAX_MANCHAS;
    if (n_manchas < MAX_MANCHAS) n_manchas++;
}

static void sangue(float x, float y, int qtd)
{
    int k;
    float a, v;
    for (k = 0; k < qtd; k++) {
        a = aleat(0, DOIS_PI);
        v = aleat(1, 5);
        criar_particula(x, y, (float)cos(a) * v, (float)sin(a) * v,
                        aleat(1.5f, 3.5f), aleat_int(15, 35), 0);
    }
    if (qtd > 2) mancha_chao(x + aleat(-8, 8), y + aleat(-8, 8), aleat_int(3, 7));
}

/* Fumaca: se grande=1, e a nuvem avassaladora do impacto em alta velocidade */
static void fumaca(float x, float y, int qtd, int grande)
{
    int k;
    float a, v;
    for (k = 0; k < qtd; k++) {
        a = aleat(0, DOIS_PI);
        v = aleat(0.5f, grande ? 5.0f : 2.0f);
        criar_particula(x, y, (float)cos(a) * v, (float)sin(a) * v,
                        grande ? aleat(10, 24) : aleat(5, 10),
                        grande ? aleat_int(50, 110) : aleat_int(25, 50), 1);
    }
}

static void destruir_obstaculo(int i)
{
    int k;
    float cx = obst[i].x + obst[i].w / 2, cy = obst[i].y + obst[i].h / 2;
    obst[i].vivo = 0;
    for (k = 0; k < 10; k++)
        criar_particula(cx, cy, aleat(-4, 4), aleat(-4, 4), 2, aleat_int(10, 25), 3);
}

static void criar_projetil(float x, float y, float vx, float vy, int vida, int dono, int dano)
{
    int i;
    for (i = 0; i < MAX_PROJ; i++) {
        if (!proj[i].ativo) {
            proj[i].x = x; proj[i].y = y;
            proj[i].vx = vx; proj[i].vy = vy;
            proj[i].vida = vida; proj[i].dono = dono;
            proj[i].dano = dano; proj[i].ativo = 1;
            return;
        }
    }
}

/* ---------------- REGRAS: MORTE, DANO, XP ---------------- */

/* Soma XP, combo e conta o louco capturado */
static void registrar_morte(int i, int mult)
{
    xp += XP_POR_KILL * mult;
    kills++;
    combo++;
    combo_timer = COMBO_TEMPO;
    if (combo > maior_combo) maior_combo = combo;

    /* A cada 100 kills de combo, recupera uma vida */
    if (combo % 100 == 0 && jog.vidas < VIDAS_INICIAIS) {
        jog.vidas++;
        strcpy(mensagem, "COMBO 100! VIDA RECUPERADA!");
        msg_timer = 180;
    }

    if (inim[i].louco) {
        loucos_capt++;
        if (loucos_capt >= loucos_total) {
            estado = ESTADO_VITORIA;
            xp += tempo_restante / 30;
            strcpy(motivo, "Todos os loucos foram capturados!");
        }
    }
}

/* Inimigo leva dano: se sobrar vida, cai de joelho no mesmo lugar */
static void dano_inimigo(int i, int dano, int ajoelhar)
{
    inim[i].hp -= dano;
    sangue(inim[i].x, inim[i].y, 6);
    if (inim[i].hp <= 0) {
        registrar_morte(i, 1);
        inim[i].vivo = 0;
        mancha_chao(inim[i].x, inim[i].y, 12);
        sangue(inim[i].x, inim[i].y, 10);
    } else {
        inim[i].estado = 1;
        inim[i].timer = ajoelhar;
    }
}

static void jogador_levar_dano(int d)
{
    if (jog.invul > 0 || jog.parry > 0) return;
    jog.hp -= d;
    combo = 0;
    tremor = 10;
    jog.invul = 25;
    sangue(jog.x, jog.y, 10);
    if (jog.hp <= 0) {
        jog.vidas--;
        if (jog.vidas <= 0) {
            estado = ESTADO_GAMEOVER;
            strcpy(motivo, "Voce morreu!");
        } else {
            jog.hp = HP_MAX;
            jog.invul = 150;
            jog.invisivel = 120;
            strcpy(mensagem, "VOCE PERDEU UMA VIDA!");
            msg_timer = 150;
        }
    }
}

/* ---------------- CRIACAO DO MAPA ---------------- */

static void add_obst(float x, float y, float w, float h, int tipo, int hp, COLORREF cor)
{
    if (n_obst >= MAX_OBST) return;
    obst[n_obst].x = x; obst[n_obst].y = y;
    obst[n_obst].w = w; obst[n_obst].h = h;
    obst[n_obst].tipo = tipo; obst[n_obst].hp = hp;
    obst[n_obst].cor = cor; obst[n_obst].vivo = 1;
    n_obst++;
}

static COLORREF cor_parede(int estilo)
{
    switch (estilo) {
        case 0: return RGB(125, 80, 52);    /* casa japonesa */
        case 1: return RGB(185, 170, 140);  /* casa europeia */
        case 2: return RGB(225, 228, 235);  /* hospital */
        default: return RGB(55, 140, 75);   /* casa verde */
    }
}

/* Parede horizontal (com ou sem porta no meio) */
static void parede_h(float x, float y, float w, int porta, int estilo)
{
    COLORREF c = cor_parede(estilo);
    if (porta) {
        add_obst(x, y, (w - LARG_PORTA) / 2, ESPESSURA, 0, 0, c);
        add_obst(x + (w + LARG_PORTA) / 2, y, (w - LARG_PORTA) / 2, ESPESSURA, 0, 0, c);
    } else {
        add_obst(x, y, w, ESPESSURA, 0, 0, c);
    }
}

/* Parede vertical (com ou sem porta no meio) */
static void parede_v(float x, float y, float h, int porta, int estilo)
{
    COLORREF c = cor_parede(estilo);
    if (porta) {
        add_obst(x, y, ESPESSURA, (h - LARG_PORTA) / 2, 0, 0, c);
        add_obst(x, y + (h + LARG_PORTA) / 2, ESPESSURA, (h - LARG_PORTA) / 2, 0, 0, c);
    } else {
        add_obst(x, y, ESPESSURA, h, 0, 0, c);
    }
}

/* Constroi uma casa: porta = 0 topo, 1 direita, 2 base, 3 esquerda */
static void criar_predio(float x, float y, float w, float h, int estilo, int porta)
{
    if (n_predios < MAX_PREDIOS) {
        predios[n_predios].x = x; predios[n_predios].y = y;
        predios[n_predios].w = w; predios[n_predios].h = h;
        predios[n_predios].estilo = estilo;
        n_predios++;
    }
    parede_h(x, y, w, porta == 0, estilo);
    parede_h(x, y + h - ESPESSURA, w, porta == 2, estilo);
    parede_v(x, y + ESPESSURA, h - 2 * ESPESSURA, porta == 3, estilo);
    parede_v(x + w - ESPESSURA, y + ESPESSURA, h - 2 * ESPESSURA, porta == 1, estilo);
    /* paredes internas com porta, para ter comodos */
    if (w >= 240) parede_v(x + w * 0.3f, y + ESPESSURA, h - 2 * ESPESSURA, 1, estilo);
    if (w >= 400) parede_v(x + w * 0.7f, y + ESPESSURA, h - 2 * ESPESSURA, 1, estilo);
}

/* Itens de rua: poste de madeira (indestrutivel), poste de ferro, lixeira, placa */
static void adicionar_prop(float x, float y, int k)
{
    int t = 1 + (k % 4);
    if (t == 1) add_obst(x, y, 10, 10, 1, 0, RGB(105, 72, 40));
    else if (t == 2) add_obst(x, y, 10, 10, 2, 3, RGB(95, 105, 120));
    else if (t == 3) add_obst(x, y, 16, 16, 3, 1, RGB(60, 110, 90));
    else add_obst(x, y, 14, 14, 4, 1, RGB(225, 190, 40));
}

static void criar_mapa(void)
{
    int x, y, k = 0;
    n_obst = 0;
    n_predios = 0;

    /* bordas do mapa */
    add_obst(0, 0, MAPA_W, 30, 0, 0, RGB(30, 30, 34));
    add_obst(0, MAPA_H - 30, MAPA_W, 30, 0, 0, RGB(30, 30, 34));
    add_obst(0, 0, 30, MAPA_H, 0, 0, RGB(30, 30, 34));
    add_obst(MAPA_W - 30, 0, 30, MAPA_H, 0, 0, RGB(30, 30, 34));

    /* fileira de cima */
    criar_predio(80, 80, 220, 170, 0, 2);
    criar_predio(340, 80, 200, 170, 1, 2);
    criar_predio(580, 80, 160, 170, 0, 2);
    criar_predio(980, 70, 440, 230, 2, 2);      /* hospital */
    criar_predio(1680, 80, 200, 170, 1, 2);
    criar_predio(1940, 80, 180, 170, 0, 2);
    criar_predio(2180, 80, 150, 170, 1, 2);

    /* fileira do meio */
    criar_predio(80, 600, 220, 200, 0, 1);
    criar_predio(360, 600, 240, 200, 1, 2);
    criar_predio(80, 880, 220, 200, 1, 1);
    criar_predio(360, 880, 240, 300, 0, 3);
    criar_predio(1020, 680, 360, 300, 3, 2);    /* a Casa Verde */
    criar_predio(1680, 600, 260, 200, 1, 3);
    criar_predio(2020, 600, 280, 200, 0, 3);
    criar_predio(1680, 900, 260, 260, 0, 3);
    criar_predio(2020, 900, 280, 260, 1, 3);

    /* fileira de baixo */
    criar_predio(80, 1420, 240, 260, 0, 0);
    criar_predio(380, 1420, 260, 260, 1, 0);
    criar_predio(940, 1420, 200, 260, 0, 0);
    criar_predio(1200, 1420, 260, 260, 1, 0);
    criar_predio(1680, 1420, 260, 260, 1, 0);
    criar_predio(2000, 1420, 300, 260, 0, 0);

    /* itens nas calcadas das ruas horizontais */
    for (x = 100; x < MAPA_W - 60; x += 200) {
        if ((x > 740 && x < 940) || (x > 1460 && x < 1660)) continue;
        adicionar_prop((float)x, 426, k++);
        adicionar_prop((float)(x + 60), 528, k++);
        adicionar_prop((float)x, 1266, k++);
        adicionar_prop((float)(x + 60), 1368, k++);
    }
    /* itens nas calcadas das ruas verticais */
    for (y = 100; y < MAPA_H - 60; y += 200) {
        if ((y > 380 && y < 580) || (y > 1220 && y < 1420)) continue;
        adicionar_prop(786, (float)y, k++);
        adicionar_prop(888, (float)(y + 60), k++);
        adicionar_prop(1506, (float)y, k++);
        adicionar_prop(1608, (float)(y + 60), k++);
    }
}

/* ---------------- INIMIGOS: CRIACAO ---------------- */

/* Cria um inimigo em lugar livre longe do jogador. Retorna 1 se conseguiu */
static int spawn_inimigo(int louco, float dist_min)
{
    int i, tent, r, tipo;
    float x = 0, y = 0;

    for (i = 0; i < MAX_INIM; i++)
        if (!inim[i].vivo) break;
    if (i >= MAX_INIM) return 0;

    for (tent = 0; tent < 300; tent++) {
        x = aleat(60, MAPA_W - 60);
        y = aleat(60, MAPA_H - 60);
        if (obstaculo_em(x, y, 18) >= 0) continue;
        if (distancia(x, y, jog.x, jog.y) < dist_min) continue;
        break;
    }
    if (tent >= 300) return 0;

    r = rand() % 100;
    if (r < 45) tipo = 0;
    else if (r < 70) tipo = 1;
    else if (r < 88) tipo = 2;
    else tipo = 3;

    memset(&inim[i], 0, sizeof(Inimigo));
    inim[i].x = x; inim[i].y = y;
    inim[i].tipo = tipo;
    inim[i].hp = hp_tipo[tipo];
    inim[i].vivo = 1;
    inim[i].louco = louco;
    inim[i].ang = aleat(0, DOIS_PI);
    inim[i].cd_ataque = 30;
    inim[i].cd_tiro = aleat_int(60, 160);
    return 1;
}

static void atualizar_camera(void)
{
    cam_x = limitar(jog.x - LARGURA / 2, 0, MAPA_W - LARGURA);
    cam_y = limitar(jog.y - ALTURA / 2, 0, MAPA_H - ALTURA);
}

/* Prepara uma partida nova */
static void iniciar_jogo(void)
{
    int n, tent, i;

    memset(inim, 0, sizeof(inim));
    memset(proj, 0, sizeof(proj));
    memset(part, 0, sizeof(part));
    memset(manchas, 0, sizeof(manchas));
    n_manchas = 0; idx_mancha = 0; idx_part = 0;
    criar_mapa();

    memset(&jog, 0, sizeof(jog));
    jog.x = 840; jog.y = 480;
    jog.hp = HP_MAX;
    jog.vidas = VIDAS_INICIAIS;
    jog.cartuchos = 2;
    jog.invul = 60;

    carro.x = 840; carro.y = 620;
    carro.ang = -PI / 2;
    carro.vel = 0;

    xp = 0; kills = 0; combo = 0; maior_combo = 0; combo_timer = 0;
    loucos_capt = 0; tremor = 0;
    tempo_restante = tempo_dif[dificuldade] * 60;

    /* cria os loucos (os alvos da missao) */
    n = 0; tent = 0;
    while (n < 5 + extra_loucos[dificuldade] && tent < 3000) {
        if (spawn_inimigo(1, 350)) n++;
        tent++;
    }
    loucos_total = n;

    /* cria os monstros comuns iniciais */
    for (i = 0; i < cap_comuns[dificuldade] / 2; i++) spawn_inimigo(0, 350);

    spawn_timer = 120;
    strcpy(mensagem, "CACE TODOS OS LOUCOS!");
    msg_timer = 180;
    strcpy(motivo, "");
    atualizar_camera();
    estado = ESTADO_JOGANDO;
}

/* ---------------- HABILIDADES DO JOGADOR ---------------- */

/* Joga um projetil inimigo de volta (parry) */
static void refletir(int k)
{
    proj[k].dono = 2;
    proj[k].vx = (float)cos(jog.mira) * 8;
    proj[k].vy = (float)sin(jog.mira) * 8;
    proj[k].dano = 8;
    proj[k].vida = 60;
}

static void usar_alma(void)
{
    if (jog.no_carro || jog.cd_alma > 0) return;
    jog.sugando = ALMA_DURACAO;
    jog.cd_alma = ALMA_RECARGA;
    tremor = 6;
}

static void usar_invis(void)
{
    if (jog.no_carro || jog.cd_invis > 0) return;
    jog.invisivel = INVIS_DURACAO;
    jog.cd_invis = INVIS_RECARGA;
}

static void usar_parry(void)
{
    int i;
    float d, a, ang;
    if (jog.no_carro || jog.cd_parry > 0) return;
    jog.parry = PARRY_JANELA;
    jog.cd_parry = PARRY_RECARGA;

    /* inimigos coladinhos levam dano e sao empurrados */
    for (i = 0; i < MAX_INIM; i++) {
        if (!inim[i].vivo || inim[i].estado == 2) continue;
        d = distancia(inim[i].x, inim[i].y, jog.x, jog.y);
        if (d < 62) {
            a = (float)atan2(inim[i].y - jog.y, inim[i].x - jog.x);
            mover_com_colisao(&inim[i].x, &inim[i].y,
                              (float)cos(a) * 18, (float)sin(a) * 18,
                              raio_tipo[inim[i].tipo]);
            dano_inimigo(i, 4, 80);
        }
    }
    /* bolas de fogo proximas voltam contra o inimigo */
    for (i = 0; i < MAX_PROJ; i++) {
        if (proj[i].ativo && proj[i].dono == 1 &&
            distancia(proj[i].x, proj[i].y, jog.x, jog.y) < 90)
            refletir(i);
    }
    for (i = 0; i < 10; i++) {
        ang = jog.mira + aleat(-1.2f, 1.2f);
        criar_particula(jog.x + (float)cos(jog.mira) * 26, jog.y + (float)sin(jog.mira) * 26,
                        (float)cos(ang) * aleat(1, 4), (float)sin(ang) * aleat(1, 4),
                        2, aleat_int(6, 12), 3);
    }
    tremor = 6;
}

/* Tiro da escopeta de cano serrado: 8 chumbos em leque */
static void atirar(void)
{
    int k;
    float a, v, mx, my;
    if (jog.no_carro || jog.cd_tiro > 0 || jog.recarga > 0 || jog.cartuchos <= 0) return;
    jog.cartuchos--;
    jog.cd_tiro = 14;
    jog.invisivel = 0; /* atirar revela o jogador */
    if (jog.cartuchos <= 0) jog.recarga = 80;

    mx = jog.x + (float)cos(jog.mira) * 22;
    my = jog.y + (float)sin(jog.mira) * 22;
    for (k = 0; k < 8; k++) {
        a = jog.mira + aleat(-0.24f, 0.24f);
        v = aleat(12, 14);
        criar_projetil(mx, my, (float)cos(a) * v, (float)sin(a) * v, 24, 0, 1);
    }
    for (k = 0; k < 5; k++) {
        a = jog.mira + aleat(-0.5f, 0.5f);
        v = aleat(2, 6);
        criar_particula(mx, my, (float)cos(a) * v, (float)sin(a) * v, 2, aleat_int(4, 8), 3);
    }
    tremor = 5;
}

/* Entra ou sai do carro (tecla Q) */
static void alternar_carro(void)
{
    int k;
    float a, nx, ny;
    if (jog.no_carro) {
        if (fabs(carro.vel) > 20) return; /* precisa parar para sair */
        for (k = 0; k < 8; k++) {
            a = carro.ang + PI / 2 + k * PI / 4;
            nx = carro.x + (float)cos(a) * 32;
            ny = carro.y + (float)sin(a) * 32;
            if (obstaculo_em(nx, ny, JOG_RAIO) < 0) {
                jog.x = nx; jog.y = ny;
                jog.no_carro = 0;
                return;
            }
        }
    } else if (distancia(jog.x, jog.y, carro.x, carro.y) < 60) {
        jog.no_carro = 1;
        jog.invisivel = 0;
        jog.x = carro.x; jog.y = carro.y;
    }
}

/* ---------------- ATUALIZACAO DO JOGO ---------------- */

static void atualizar_jogador(void)
{
    float dx = 0, dy = 0, n, d;
    int i;

    /* mira apontada pelo mouse (em coordenadas do mundo) */
    jog.mira = (float)atan2((mouse_y + cam_y) - jog.y, (mouse_x + cam_x) - jog.x);

    /* contadores de recarga */
    if (jog.cd_tiro > 0) jog.cd_tiro--;
    if (jog.recarga > 0) {
        jog.recarga--;
        if (jog.recarga == 0) jog.cartuchos = 2;
    }
    if (jog.cd_invis > 0) jog.cd_invis--;
    if (jog.cd_parry > 0) jog.cd_parry--;
    if (jog.cd_alma > 0) jog.cd_alma--;
    if (jog.invisivel > 0) jog.invisivel--;
    if (jog.parry > 0) jog.parry--;
    if (jog.invul > 0) jog.invul--;

    /* movimento a pe */
    if (!jog.no_carro) {
        if (teclas['W']) dy -= 1;
        if (teclas['S']) dy += 1;
        if (teclas['A']) dx -= 1;
        if (teclas['D']) dx += 1;
        n = (float)sqrt(dx * dx + dy * dy);
        if (n > 0) {
            dx = dx / n * JOG_VEL;
            dy = dy / n * JOG_VEL;
            mover_com_colisao(&jog.x, &jog.y, dx, dy, JOG_RAIO);
        }
        if (mouse_botao) atirar();
    }

    /* sugar almas: puxa e mata os inimigos proximos */
    if (jog.sugando > 0) {
        jog.sugando--;
        for (i = 0; i < MAX_INIM; i++) {
            if (!inim[i].vivo || inim[i].estado == 2) continue;
            d = distancia(inim[i].x, inim[i].y, jog.x, jog.y);
            if (d < ALMA_RAIO) {
                inim[i].estado = 1;
                inim[i].timer = 6;
                if (d < 24) {
                    for (n = 0; n < 8; n++)
                        criar_particula(inim[i].x, inim[i].y, aleat(-3, 3), aleat(-3, 3),
                                        3, aleat_int(10, 25), 2);
                    registrar_morte(i, 1);
                    inim[i].vivo = 0;
                    jog.hp += 8;
                    if (jog.hp > HP_MAX) jog.hp = HP_MAX;
                } else {
                    dx = (jog.x - inim[i].x) / d * 5;
                    dy = (jog.y - inim[i].y) / d * 5;
                    mover_com_colisao(&inim[i].x, &inim[i].y, dx, dy, raio_tipo[inim[i].tipo]);
                    if (frame % 2 == 0)
                        criar_particula(inim[i].x, inim[i].y, dx * 0.5f, dy * 0.5f,
                                        2.5f, 15, 2);
                }
            }
        }
    }
}

static void atualizar_carro(void)
{
    float nx, ny, passo, dir, fator, vant, vabs;
    int idx, i, mult;

    if (jog.no_carro) {
        /* acelerar, frear e dar re */
        if (teclas['W']) carro.vel += 1.4f;
        else if (teclas['S']) {
            if (carro.vel > 0) carro.vel -= 2.6f;
            else carro.vel -= 0.9f;
        } else {
            if (carro.vel > 0.6f) carro.vel -= 0.6f;
            else if (carro.vel < -0.6f) carro.vel += 0.6f;
            else carro.vel = 0;
        }
        /* virar o volante */
        dir = 0;
        if (teclas['A']) dir -= 1;
        if (teclas['D']) dir += 1;
        fator = limitar(carro.vel / 40.0f, -1.0f, 1.0f);
        carro.ang += dir * 0.052f * fator;
    } else {
        carro.vel *= 0.9f;
    }
    carro.vel = limitar(carro.vel, -30, 120);

    /* movimento e colisao com o cenario */
    passo = carro.vel * 0.06f;
    nx = carro.x + (float)cos(carro.ang) * passo;
    ny = carro.y + (float)sin(carro.ang) * passo;
    idx = obstaculo_em(nx, ny, CARRO_RAIO);
    if (idx < 0) {
        carro.x = nx; carro.y = ny;
    } else {
        vant = (float)fabs(carro.vel);
        if (obst[idx].tipo >= 2 && vant > 30) {
            destruir_obstaculo(idx);
            carro.vel *= 0.85f;
            carro.x = nx; carro.y = ny;
        } else {
            carro.vel = -carro.vel * 0.3f;
            if (vant > 60) {
                tremor = 12;
                fumaca(nx, ny, 6, 0);
                if (jog.no_carro) jogador_levar_dano((int)(vant / 6));
            }
        }
    }

    /* fumaca do escapamento em alta velocidade */
    if (jog.no_carro && fabs(carro.vel) > 90 && frame % 4 == 0)
        criar_particula(carro.x - (float)cos(carro.ang) * 24, carro.y - (float)sin(carro.ang) * 24,
                        0, 0, 4, 25, 1);

    /* atropelamento */
    vabs = (float)fabs(carro.vel);
    if (jog.no_carro && vabs > 20) {
        for (i = 0; i < MAX_INIM; i++) {
            if (!inim[i].vivo || inim[i].estado == 2) continue;
            if (distancia(carro.x, carro.y, inim[i].x, inim[i].y) < CARRO_RAIO + raio_tipo[inim[i].tipo]) {
                mult = mult_vel(vabs);
                registrar_morte(i, mult);
                inim[i].estado = 2;               /* sera lancado ate a parede */
                inim[i].vel_voo = vabs;
                inim[i].timer = 100;
                inim[i].ang = (carro.vel > 0) ? carro.ang : carro.ang + PI;
                sangue(inim[i].x, inim[i].y, 12);
                carro.vel *= 0.98f;
                if (mult > 1) {
                    sprintf(mensagem, "ATROPELOU! x%d  +%d XP", mult, XP_POR_KILL * mult);
                    msg_timer = 70;
                }
            }
        }
    }

    if (jog.no_carro) {
        jog.x = carro.x;
        jog.y = carro.y;
    }
}

static void atualizar_inimigos(void)
{
    int i, k, t, n, s, ve;
    float d, v, passo, fator, rr, dx, dy, ox, oy, ang;

    fator = 1.0f + 0.07f * dificuldade;

    for (i = 0; i < MAX_INIM; i++) {
        if (!inim[i].vivo) continue;
        t = inim[i].tipo;

        /* corpo lancado pelo atropelamento: voa ate bater na parede */
        if (inim[i].estado == 2) {
            passo = inim[i].vel_voo * 0.16f;
            n = (int)(passo / 6.0f) + 1;
            for (s = 0; s < n; s++) {
                inim[i].x += (float)cos(inim[i].ang) * passo / n;
                inim[i].y += (float)sin(inim[i].ang) * passo / n;
                k = obstaculo_em(inim[i].x, inim[i].y, 7);
                if (k >= 0) {
                    if (inim[i].vel_voo >= 72) {   /* fumaca avassaladora */
                        fumaca(inim[i].x, inim[i].y, 45, 1);
                        tremor = 16;
                    } else {
                        fumaca(inim[i].x, inim[i].y, 10, 0);
                    }
                    if (obst[k].tipo >= 2) destruir_obstaculo(k);
                    mancha_chao(inim[i].x, inim[i].y, 16);
                    sangue(inim[i].x, inim[i].y, 10);
                    inim[i].vivo = 0;
                    break;
                }
            }
            inim[i].giro += 0.5f;
            inim[i].timer--;
            if (inim[i].timer <= 0) inim[i].vivo = 0;
            continue;
        }

        if (inim[i].cd_ataque > 0) inim[i].cd_ataque--;

        /* ajoelhado: fica parado jorrando sangue */
        if (inim[i].estado == 1) {
            inim[i].timer--;
            if (inim[i].timer % 6 == 0) sangue(inim[i].x, inim[i].y, 2);
            if (inim[i].timer <= 0) inim[i].estado = 0;
            continue;
        }

        d = distancia(inim[i].x, inim[i].y, jog.x, jog.y);
        ve = (jog.invisivel <= 0 && d < (inim[i].louco ? 380.0f : 420.0f));
        v = vel_tipo[t] * fator;
        if (inim[i].louco) v *= 1.15f;
        ang = inim[i].ang;

        if (ve) {
            ang = (float)atan2(jog.y - inim[i].y, jog.x - inim[i].x);
            if (inim[i].louco) ang += (float)sin(frame * 0.15 + i) * 0.6f; /* anda torto */
            if (t == 1) { /* vampiro: investida rapida */
                if (inim[i].dash > 0) { v *= 2.3f; inim[i].dash--; }
                else if (d < 280 && rand() % 200 == 0) inim[i].dash = 25;
            }
            if (t == 3) { /* demonio: mantem distancia e atira */
                if (d < 200) v = -v * 0.7f;
                else if (d < 260) v = 0;
            }
        } else {
            inim[i].dir_timer--;
            if (inim[i].dir_timer <= 0) {
                inim[i].ang = aleat(0, DOIS_PI);
                inim[i].dir_timer = aleat_int(60, 180);
            }
            ang = inim[i].ang;
            v *= 0.4f;
        }

        ox = inim[i].x; oy = inim[i].y;
        mover_com_colisao(&inim[i].x, &inim[i].y,
                          (float)cos(ang) * v, (float)sin(ang) * v, raio_tipo[t]);
        if (ox == inim[i].x && oy == inim[i].y) {
            if (ve) { /* travou na parede: tenta contornar */
                ang += (i % 2 == 0) ? PI / 2 : -PI / 2;
                mover_com_colisao(&inim[i].x, &inim[i].y,
                                  (float)cos(ang) * fabs(v), (float)sin(ang) * fabs(v), raio_tipo[t]);
            } else {
                inim[i].dir_timer = 0;
            }
        }

        /* ataque corpo a corpo */
        rr = raio_tipo[t] + (jog.no_carro ? CARRO_RAIO : JOG_RAIO) + 3;
        if (d < rr && inim[i].cd_ataque <= 0 && jog.invisivel <= 0) {
            if (jog.parry > 0 && !jog.no_carro) { /* parry: o golpe volta no inimigo */
                inim[i].cd_ataque = 60;
                dano_inimigo(i, 4, 90);
                for (k = 0; k < 6; k++)
                    criar_particula(jog.x, jog.y, aleat(-3, 3), aleat(-3, 3), 2, aleat_int(6, 12), 3);
            } else {
                jogador_levar_dano(dano_tipo[t]);
                inim[i].cd_ataque = 45;
            }
        }

        /* demonio atira bola de fogo */
        if (inim[i].vivo && t == 3 && ve && d < 400) {
            inim[i].cd_tiro--;
            if (inim[i].cd_tiro <= 0) {
                dx = (float)cos(ang) * 4.5f;
                dy = (float)sin(ang) * 4.5f;
                if (v != 0 || 1) {
                    ang = (float)atan2(jog.y - inim[i].y, jog.x - inim[i].x);
                    dx = (float)cos(ang) * 4.5f;
                    dy = (float)sin(ang) * 4.5f;
                }
                criar_projetil(inim[i].x, inim[i].y, dx, dy, 120, 1, 15);
                inim[i].cd_tiro = aleat_int(110, 170);
            }
        }
    }

    /* separa inimigos para nao ficarem empilhados */
    for (i = 0; i < MAX_INIM; i++) {
        if (!inim[i].vivo || inim[i].estado == 2) continue;
        for (k = i + 1; k < MAX_INIM; k++) {
            if (!inim[k].vivo || inim[k].estado == 2) continue;
            dx = inim[k].x - inim[i].x;
            dy = inim[k].y - inim[i].y;
            d = (float)sqrt(dx * dx + dy * dy);
            if (d < 18 && d > 0.01f) {
                dx = dx / d * (18 - d) * 0.5f;
                dy = dy / d * (18 - d) * 0.5f;
                mover_com_colisao(&inim[k].x, &inim[k].y, dx, dy, raio_tipo[inim[k].tipo]);
                mover_com_colisao(&inim[i].x, &inim[i].y, -dx, -dy, raio_tipo[inim[i].tipo]);
            }
        }
    }
}

static void atualizar_projeteis(void)
{
    int i, k, s, idx;

    for (i = 0; i < MAX_PROJ; i++) {
        if (!proj[i].ativo) continue;
        proj[i].vida--;
        if (proj[i].vida <= 0) { proj[i].ativo = 0; continue; }

        /* parry: reflete bolas de fogo que chegam perto */
        if (proj[i].dono == 1 && jog.parry > 0 && !jog.no_carro &&
            distancia(proj[i].x, proj[i].y, jog.x, jog.y) < 80)
            refletir(i);

        /* anda em 2 passos para nao atravessar paredes finas */
        for (s = 0; s < 2 && proj[i].ativo; s++) {
            proj[i].x += proj[i].vx / 2;
            proj[i].y += proj[i].vy / 2;

            idx = obstaculo_em(proj[i].x, proj[i].y, 2);
            if (idx >= 0) {
                if (proj[i].dono != 1 && obst[idx].tipo >= 2) {
                    obst[idx].hp--;
                    if (obst[idx].hp <= 0) destruir_obstaculo(idx);
                }
                criar_particula(proj[i].x, proj[i].y, aleat(-2, 2), aleat(-2, 2), 1.5f, 6, 3);
                proj[i].ativo = 0;
                break;
            }

            if (proj[i].dono == 1) {
                if (distancia(proj[i].x, proj[i].y, jog.x, jog.y) <
                    (jog.no_carro ? CARRO_RAIO : JOG_RAIO) + 5) {
                    jogador_levar_dano(proj[i].dano);
                    proj[i].ativo = 0;
                    break;
                }
            } else {
                for (k = 0; k < MAX_INIM; k++) {
                    if (!inim[k].vivo || inim[k].estado == 2) continue;
                    if (distancia(proj[i].x, proj[i].y, inim[k].x, inim[k].y) < raio_tipo[inim[k].tipo] + 3) {
                        dano_inimigo(k, proj[i].dano, proj[i].dono == 2 ? 90 : 60);
                        proj[i].ativo = 0;
                        break;
                    }
                }
                if (!proj[i].ativo) break;
            }
        }
    }
}

static void atualizar_particulas(void)
{
    int i;
    for (i = 0; i < MAX_PART; i++) {
        if (!part[i].ativo) continue;
        part[i].x += part[i].vx;
        part[i].y += part[i].vy;
        part[i].vx *= 0.93f;
        part[i].vy *= 0.93f;
        if (part[i].tipo == 1) part[i].raio += 0.2f;
        part[i].vida--;
        if (part[i].vida <= 0) part[i].ativo = 0;
    }
}

/* Chamada uma vez por quadro (60 vezes por segundo) */
static void atualizar_jogo(void)
{
    int i, comuns;

    frame++;
    if (estado != ESTADO_JOGANDO) return;

    atualizar_jogador();
    atualizar_carro();
    atualizar_inimigos();
    atualizar_projeteis();
    atualizar_particulas();

    /* novos monstros comuns aparecem aos poucos */
    spawn_timer--;
    if (spawn_timer <= 0) {
        comuns = 0;
        for (i = 0; i < MAX_INIM; i++)
            if (inim[i].vivo && !inim[i].louco && inim[i].estado != 2) comuns++;
        if (comuns < cap_comuns[dificuldade]) spawn_inimigo(0, 520);
        spawn_timer = aleat_int(50, 90);
    }

    if (combo_timer > 0) {
        combo_timer--;
        if (combo_timer == 0) combo = 0;
    }
    if (tremor > 0) tremor--;
    if (msg_timer > 0) msg_timer--;

    tempo_restante--;
    if (tempo_restante <= 0 && estado == ESTADO_JOGANDO) {
        estado = ESTADO_GAMEOVER;
        strcpy(motivo, "O tempo acabou!");
    }

    atualizar_camera();
}

/* ---------------- DESENHO: FUNCOES AUXILIARES (GDI) ---------------- */

static COLORREF escurecer(COLORREF c, float f)
{
    return RGB((int)(GetRValue(c) * f), (int)(GetGValue(c) * f), (int)(GetBValue(c) * f));
}

static void retangulo(HDC hdc, int x, int y, int w, int h, COLORREF cor)
{
    RECT r;
    HBRUSH b;
    r.left = x; r.top = y; r.right = x + w; r.bottom = y + h;
    b = CreateSolidBrush(cor);
    FillRect(hdc, &r, b);
    DeleteObject(b);
}

static void elipse(HDC hdc, int cx, int cy, int rx, int ry, COLORREF cor)
{
    HBRUSH b = CreateSolidBrush(cor);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, b);
    HPEN op = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, cx - rx, cy - ry, cx + rx + 1, cy + ry + 1);
    SelectObject(hdc, op);
    SelectObject(hdc, ob);
    DeleteObject(b);
}

static void contorno(HDC hdc, int cx, int cy, int r, COLORREF cor, int esp)
{
    HPEN p = CreatePen(PS_SOLID, esp, cor);
    HPEN op = (HPEN)SelectObject(hdc, p);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Ellipse(hdc, cx - r, cy - r, cx + r + 1, cy + r + 1);
    SelectObject(hdc, ob);
    SelectObject(hdc, op);
    DeleteObject(p);
}

static void linha(HDC hdc, int x1, int y1, int x2, int y2, int esp, COLORREF cor)
{
    HPEN p = CreatePen(PS_SOLID, esp, cor);
    HPEN op = (HPEN)SelectObject(hdc, p);
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);
    SelectObject(hdc, op);
    DeleteObject(p);
}

static void poligono(HDC hdc, POINT *p, int n, COLORREF cor)
{
    HBRUSH b = CreateSolidBrush(cor);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, b);
    HPEN op = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
    Polygon(hdc, p, n);
    SelectObject(hdc, op);
    SelectObject(hdc, ob);
    DeleteObject(b);
}

/* Desenha um poligono dado em coordenadas locais (x para frente), girado por ang */
static void poli_local(HDC hdc, float cx, float cy, float ang, const float *v, int n, COLORREF cor)
{
    POINT p[24];
    int k;
    float c = (float)cos(ang), s = (float)sin(ang);
    if (n > 24) n = 24;
    for (k = 0; k < n; k++) {
        p[k].x = (LONG)(cx + v[2 * k] * c - v[2 * k + 1] * s);
        p[k].y = (LONG)(cy + v[2 * k] * s + v[2 * k + 1] * c);
    }
    poligono(hdc, p, n, cor);
}

/* Bolinha em coordenadas locais (olhos, faroes...) */
static void ponto_local(HDC hdc, int cx, int cy, float ang, float lx, float ly, int r, COLORREF cor)
{
    float c = (float)cos(ang), s = (float)sin(ang);
    elipse(hdc, cx + (int)(lx * c - ly * s), cy + (int)(lx * s + ly * c), r, r, cor);
}

static void texto(HDC hdc, int x, int y, const char *s, COLORREF cor, HFONT f, UINT alinh)
{
    HFONT antigo;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, cor);
    SetTextAlign(hdc, alinh);
    antigo = (HFONT)SelectObject(hdc, f);
    TextOutA(hdc, x, y, s, (int)strlen(s));
    SelectObject(hdc, antigo);
}

static void texto_sombra(HDC hdc, int x, int y, const char *s, COLORREF cor, HFONT f, UINT alinh)
{
    texto(hdc, x + 2, y + 2, s, RGB(0, 0, 0), f, alinh);
    texto(hdc, x, y, s, cor, f, alinh);
}

static HFONT criar_fonte(int tam, const char *nome)
{
    return CreateFontA(tam, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                       DEFAULT_PITCH | FF_DONTCARE, nome);
}

/* ---------------- DESENHO: CENARIO ---------------- */

static void desenhar_mundo(HDC hdc)
{
    int i, k, sx, sy, w, h, cx, cy;
    POINT p[4];
    COLORREF cor;
    static const int estradas[4][4] = {
        {0, 420, MAPA_W, 120}, {0, 1260, MAPA_W, 120},
        {780, 0, 120, MAPA_H}, {1500, 0, 120, MAPA_H}
    };
    static const int calcadas[8][4] = {
        {0, 420, MAPA_W, 12}, {0, 528, MAPA_W, 12},
        {0, 1260, MAPA_W, 12}, {0, 1368, MAPA_W, 12},
        {780, 0, 12, MAPA_H}, {888, 0, 12, MAPA_H},
        {1500, 0, 12, MAPA_H}, {1608, 0, 12, MAPA_H}
    };

    /* chao de terra/grama escura */
    retangulo(hdc, 0, 0, LARGURA, ALTURA, RGB(58, 72, 52));

    /* ruas e calcadas */
    for (i = 0; i < 4; i++)
        retangulo(hdc, estradas[i][0] - off_x, estradas[i][1] - off_y, estradas[i][2], estradas[i][3], RGB(72, 72, 80));
    for (i = 0; i < 8; i++)
        retangulo(hdc, calcadas[i][0] - off_x, calcadas[i][1] - off_y, calcadas[i][2], calcadas[i][3], RGB(125, 122, 115));

    /* faixas tracejadas no meio das ruas */
    for (k = 0; k < MAPA_W; k += 80) {
        if ((k >= 740 && k < 920) || (k >= 1460 && k < 1640)) continue;
        retangulo(hdc, k - off_x, 478 - off_y, 40, 4, RGB(200, 190, 110));
        retangulo(hdc, k - off_x, 1318 - off_y, 40, 4, RGB(200, 190, 110));
    }
    for (k = 0; k < MAPA_H; k += 80) {
        if ((k >= 380 && k < 560) || (k >= 1220 && k < 1400)) continue;
        retangulo(hdc, 838 - off_x, k - off_y, 4, 40, RGB(200, 190, 110));
        retangulo(hdc, 1558 - off_x, k - off_y, 4, 40, RGB(200, 190, 110));
    }

    /* pisos das construcoes */
    for (i = 0; i < n_predios; i++) {
        sx = (int)(predios[i].x - off_x);
        sy = (int)(predios[i].y - off_y);
        w = (int)predios[i].w;
        h = (int)predios[i].h;
        if (sx > LARGURA || sy > ALTURA || sx + w < 0 || sy + h < 0) continue;
        switch (predios[i].estilo) {
            case 0: cor = RGB(165, 140, 95); break;
            case 1: cor = RGB(120, 95, 70); break;
            case 2: cor = RGB(205, 215, 220); break;
            default: cor = RGB(70, 110, 80); break;
        }
        retangulo(hdc, sx, sy, w, h, cor);
        cx = sx + w / 2; cy = sy + h / 2;
        if (predios[i].estilo == 2) { /* cruz vermelha do hospital */
            retangulo(hdc, cx - 7, cy - 28, 14, 56, RGB(190, 30, 40));
            retangulo(hdc, cx - 28, cy - 7, 56, 14, RGB(190, 30, 40));
        }
        if (predios[i].estilo == 3)
            texto_sombra(hdc, cx, cy - 10, "CASA VERDE", RGB(190, 255, 200), f_media, TA_CENTER | TA_TOP);
    }

    /* manchas de sangue no chao */
    for (i = 0; i < n_manchas; i++) {
        sx = (int)(manchas[i].x - off_x);
        sy = (int)(manchas[i].y - off_y);
        if (sx < -20 || sx > LARGURA + 20 || sy < -20 || sy > ALTURA + 20) continue;
        elipse(hdc, sx, sy, manchas[i].r, manchas[i].r, RGB(105, 12, 14));
    }

    /* paredes, postes, lixeiras e placas */
    for (i = 0; i < n_obst; i++) {
        if (!obst[i].vivo) continue;
        sx = (int)(obst[i].x - off_x);
        sy = (int)(obst[i].y - off_y);
        w = (int)obst[i].w;
        h = (int)obst[i].h;
        if (sx > LARGURA || sy > ALTURA || sx + w < 0 || sy + h < 0) continue;
        switch (obst[i].tipo) {
            case 0:
            case 1:
            case 2:
                retangulo(hdc, sx, sy, w, h, escurecer(obst[i].cor, 0.55f));
                retangulo(hdc, sx + 2, sy + 2, w - 4, h - 4, obst[i].cor);
                break;
            case 3:
                elipse(hdc, sx + w / 2, sy + h / 2, w / 2, h / 2, RGB(35, 60, 50));
                elipse(hdc, sx + w / 2, sy + h / 2, w / 2 - 2, h / 2 - 2, obst[i].cor);
                break;
            default:
                p[0].x = sx + w / 2; p[0].y = sy;
                p[1].x = sx + w;     p[1].y = sy + h / 2;
                p[2].x = sx + w / 2; p[2].y = sy + h;
                p[3].x = sx;         p[3].y = sy + h / 2;
                poligono(hdc, p, 4, obst[i].cor);
                elipse(hdc, sx + w / 2, sy + h / 2, 3, 3, RGB(180, 30, 30));
                break;
        }
    }
}

/* ---------------- DESENHO: PERSONAGENS ---------------- */

static void desenhar_carro(HDC hdc)
{
    int sx = (int)(carro.x - off_x), sy = (int)(carro.y - off_y), k;
    float a = carro.ang;
    float corpo[16] = {22,-8, 14,-13, -18,-13, -22,-8, -22,8, -18,13, 14,13, 22,8};
    float cabine[12] = {8,-9, -4,-9, -14,-7, -14,7, -4,9, 8,9};
    float teto[8] = {2,-7, -10,-6, -10,6, 2,7};
    float roda[8];
    float rx[4] = {12, 12, -14, -14};
    float ry[4] = {-14, 14, -14, 14};
    float dist_jog;

    if (sx < -60 || sx > LARGURA + 60 || sy < -60 || sy > ALTURA + 60) return;

    for (k = 0; k < 4; k++) { /* rodas */
        roda[0] = rx[k] - 5; roda[1] = ry[k] - 2;
        roda[2] = rx[k] + 5; roda[3] = ry[k] - 2;
        roda[4] = rx[k] + 5; roda[5] = ry[k] + 2;
        roda[6] = rx[k] - 5; roda[7] = ry[k] + 2;
        poli_local(hdc, (float)sx, (float)sy, a, roda, 4, RGB(20, 20, 22));
    }
    poli_local(hdc, (float)sx, (float)sy, a, corpo, 8, RGB(150, 12, 18));  /* vermelho sangue */
    poli_local(hdc, (float)sx, (float)sy, a, cabine, 6, RGB(35, 40, 55));
    poli_local(hdc, (float)sx, (float)sy, a, teto, 4, RGB(120, 8, 14));
    ponto_local(hdc, sx, sy, a, 22, -7, 2, RGB(250, 230, 120));
    ponto_local(hdc, sx, sy, a, 22, 7, 2, RGB(250, 230, 120));

    if (!jog.no_carro) {
        dist_jog = distancia(jog.x, jog.y, carro.x, carro.y);
        if (dist_jog < 60)
            texto_sombra(hdc, sx, sy - 40, "[Q] ENTRAR", RGB(255, 255, 255), f_peq, TA_CENTER | TA_TOP);
    }
}

static void desenhar_inimigo(HDC hdc, int i)
{
    int sx, sy, t, r, ajo;
    float a;
    COLORREF cor;

    t = inim[i].tipo;
    r = (int)raio_tipo[t];
    sx = (int)(inim[i].x - off_x);
    sy = (int)(inim[i].y - off_y);
    if (sx < -40 || sx > LARGURA + 40 || sy < -40 || sy > ALTURA + 40) return;
    ajo = (inim[i].estado == 1);
    a = (inim[i].estado == 2) ? inim[i].giro : inim[i].ang;

    if (ajo) elipse(hdc, sx, sy, r + 5, r + 5, RGB(115, 12, 16)); /* poca de sangue */
    if (inim[i].louco && inim[i].estado != 2)
        contorno(hdc, sx, sy, r + 7 + (int)(2 * sin(frame * 0.2)), RGB(255, 225, 40), 2);

    switch (t) {
        case 0: { /* zumbi */
            float braco1[8] = {6,-10, 20,-10, 20,-6, 6,-6};
            float braco2[8] = {6,6, 20,6, 20,10, 6,10};
            cor = ajo ? RGB(70, 100, 62) : RGB(96, 134, 84);
            poli_local(hdc, (float)sx, (float)sy, a, braco1, 4, RGB(120, 155, 100));
            poli_local(hdc, (float)sx, (float)sy, a, braco2, 4, RGB(120, 155, 100));
            elipse(hdc, sx, sy, r, r, cor);
            ponto_local(hdc, sx, sy, a, 2, 0, 6, RGB(70, 100, 62));
            ponto_local(hdc, sx, sy, a, 5, -3, 2, RGB(220, 40, 40));
            ponto_local(hdc, sx, sy, a, 5, 3, 2, RGB(220, 40, 40));
            break;
        }
        case 1: { /* vampiro */
            float capa[16] = {10,0, 0,-16, -6,-9, -10,-14, -14,0, -10,14, -6,9, 0,16};
            poli_local(hdc, (float)sx, (float)sy, a, capa, 8, ajo ? RGB(52, 12, 70) : RGB(70, 16, 92));
            ponto_local(hdc, sx, sy, a, 2, 0, 6, RGB(228, 218, 222));
            ponto_local(hdc, sx, sy, a, 5, -2, 1, RGB(230, 20, 30));
            ponto_local(hdc, sx, sy, a, 5, 2, 1, RGB(230, 20, 30));
            break;
        }
        case 2: { /* lobisomem */
            float o1[6] = {6,-11, 12,-17, 13,-8};
            float o2[6] = {6,11, 12,17, 13,8};
            float foc[6] = {10,-4, 20,0, 10,4};
            float g1[6] = {14,-13, 24,-11, 14,-9};
            float g2[6] = {14,13, 24,11, 14,9};
            cor = ajo ? RGB(72, 46, 30) : RGB(98, 64, 42);
            poli_local(hdc, (float)sx, (float)sy, a, g1, 3, RGB(220, 220, 210));
            poli_local(hdc, (float)sx, (float)sy, a, g2, 3, RGB(220, 220, 210));
            elipse(hdc, sx, sy, r, r, cor);
            poli_local(hdc, (float)sx, (float)sy, a, o1, 3, RGB(55, 35, 22));
            poli_local(hdc, (float)sx, (float)sy, a, o2, 3, RGB(55, 35, 22));
            poli_local(hdc, (float)sx, (float)sy, a, foc, 3, RGB(70, 45, 30));
            ponto_local(hdc, sx, sy, a, 6, -4, 2, RGB(250, 220, 40));
            ponto_local(hdc, sx, sy, a, 6, 4, 2, RGB(250, 220, 40));
            break;
        }
        default: { /* demonio */
            float h1[6] = {6,-9, 16,-16, 12,-5};
            float h2[6] = {6,9, 16,16, 12,5};
            cor = ajo ? RGB(140, 28, 20) : RGB(190, 40, 28);
            elipse(hdc, sx, sy, r, r, cor);
            poli_local(hdc, (float)sx, (float)sy, a, h1, 3, RGB(40, 10, 10));
            poli_local(hdc, (float)sx, (float)sy, a, h2, 3, RGB(40, 10, 10));
            ponto_local(hdc, sx, sy, a, 7, -4, 2, RGB(255, 190, 40));
            ponto_local(hdc, sx, sy, a, 7, 4, 2, RGB(255, 190, 40));
            break;
        }
    }
}

static void desenhar_jogador(HDC hdc)
{
    int sx = (int)(jog.x - off_x), sy = (int)(jog.y - off_y), k;
    float a = jog.mira, ar, t;
    float rabo[6] = {-15,0, -4,-9, -4,9};
    float arma[8] = {8,1, 30,1, 30,8, 8,8};
    float cano[8] = {12,3, 30,3, 30,5, 12,5};
    float cabelo[32];
    float lamina[6] = {8,-2, 36,0, 8,2};

    if (jog.no_carro) return;
    if (jog.invul > 0 && (frame / 3) % 2 == 0) return; /* pisca ao levar dano */

    /* invisivel: so um contorno fantasma */
    if (jog.invisivel > 0) {
        if (jog.invisivel > 40 || (frame / 3) % 2 == 0)
            contorno(hdc, sx, sy, JOG_RAIO + 2, RGB(140, 200, 255), 1);
        return;
    }

    poli_local(hdc, (float)sx, (float)sy, a, rabo, 3, RGB(18, 18, 24));       /* ponta do capuz */
    poli_local(hdc, (float)sx, (float)sy, a, arma, 4, RGB(190, 172, 105));    /* escopeta amarelada */
    poli_local(hdc, (float)sx, (float)sy, a, cano, 4, RGB(120, 120, 125));    /* cano cinza */
    elipse(hdc, sx, sy, JOG_RAIO, JOG_RAIO, RGB(28, 28, 36));                 /* capuz escuro */

    /* cabelo amarelo espetado (estrela de 8 pontas) */
    for (k = 0; k < 16; k++) {
        t = (k % 2 == 0) ? 8.0f : 4.0f;
        cabelo[2 * k] = -2 + (float)cos(k * PI / 8) * t;
        cabelo[2 * k + 1] = (float)sin(k * PI / 8) * t;
    }
    poli_local(hdc, (float)sx, (float)sy, a, cabelo, 16, RGB(240, 205, 40));

    ponto_local(hdc, sx, sy, a, 7.5f, -3.5f, 2, RGB(105, 70, 45)); /* olhos castanhos */
    ponto_local(hdc, sx, sy, a, 7.5f, 3.5f, 2, RGB(105, 70, 45));

    /* faca do parry girando em arco */
    if (jog.parry > 0) {
        ar = a - 1.3f + 2.6f * (1.0f - (float)jog.parry / PARRY_JANELA);
        for (k = 0; k < 6; k++) {
            t = a - 1.3f + 2.6f * k / 5.0f;
            if (t <= ar)
                linha(hdc, sx + (int)(cos(t) * 30), sy + (int)(sin(t) * 30),
                      sx + (int)(cos(t) * 36), sy + (int)(sin(t) * 36), 3, RGB(230, 230, 240));
        }
        poli_local(hdc, (float)sx, (float)sy, ar, lamina, 3, RGB(215, 215, 205));
    }
}

static void desenhar_projeteis(HDC hdc)
{
    int i, sx, sy;
    for (i = 0; i < MAX_PROJ; i++) {
        if (!proj[i].ativo) continue;
        sx = (int)(proj[i].x - off_x);
        sy = (int)(proj[i].y - off_y);
        if (sx < -20 || sx > LARGURA + 20 || sy < -20 || sy > ALTURA + 20) continue;
        if (proj[i].dono == 0) {
            linha(hdc, sx - (int)proj[i].vx, sy - (int)proj[i].vy, sx, sy, 2, RGB(255, 235, 140));
        } else if (proj[i].dono == 1) {
            elipse(hdc, sx, sy, 6, 6, RGB(255, 140, 30));
            elipse(hdc, sx, sy, 3, 3, RGB(255, 235, 120));
        } else {
            elipse(hdc, sx, sy, 6, 6, RGB(120, 230, 255));
            elipse(hdc, sx, sy, 3, 3, RGB(255, 255, 255));
        }
    }
}

static void desenhar_particulas(HDC hdc)
{
    int i, sx, sy, g;
    float t;
    for (i = 0; i < MAX_PART; i++) {
        if (!part[i].ativo) continue;
        sx = (int)(part[i].x - off_x);
        sy = (int)(part[i].y - off_y);
        if (sx < -40 || sx > LARGURA + 40 || sy < -40 || sy > ALTURA + 40) continue;
        t = (float)part[i].vida / (float)part[i].vida_max;
        switch (part[i].tipo) {
            case 0:
                elipse(hdc, sx, sy, (int)part[i].raio, (int)part[i].raio, RGB(100 + (int)(60 * t), 8, 10));
                break;
            case 1:
                g = 66 + (int)(t * 80);
                elipse(hdc, sx, sy, (int)part[i].raio, (int)part[i].raio, RGB(g, g, g + 4));
                break;
            case 2:
                elipse(hdc, sx, sy, (int)part[i].raio, (int)part[i].raio, RGB(110, 255, 255));
                break;
            default:
                elipse(hdc, sx, sy, (int)part[i].raio, (int)part[i].raio, RGB(255, 220, 80));
                break;
        }
    }
}

/* Seta amarela na borda da tela apontando o louco mais proximo */
static void desenhar_radar(HDC hdc)
{
    int i, melhor = -1;
    float d, dm = 999999.0f, a, sx, sy, px, py;
    float seta[6] = {14,0, -8,-9, -8,9};

    for (i = 0; i < MAX_INIM; i++) {
        if (!inim[i].vivo || !inim[i].louco || inim[i].estado == 2) continue;
        d = distancia(inim[i].x, inim[i].y, jog.x, jog.y);
        if (d < dm) { dm = d; melhor = i; }
    }
    if (melhor < 0) return;
    sx = inim[melhor].x - off_x;
    sy = inim[melhor].y - off_y;
    if (sx > 30 && sx < LARGURA - 30 && sy > 30 && sy < ALTURA - 30) return;
    a = (float)atan2(sy - ALTURA / 2, sx - LARGURA / 2);
    px = LARGURA / 2 + (float)cos(a) * (LARGURA / 2 - 36);
    py = ALTURA / 2 + (float)sin(a) * (ALTURA / 2 - 36);
    poli_local(hdc, px, py, a, seta, 3, RGB(255, 225, 40));
}

/* ---------------- DESENHO: INTERFACE (HUD) ---------------- */

static void barra_hab(HDC hdc, int x, int y, const char *nome, int cd, int cdmax)
{
    int w = (cdmax - cd) * 146 / cdmax;
    retangulo(hdc, x, y, 150, 20, RGB(10, 10, 10));
    retangulo(hdc, x + 2, y + 2, w, 16, cd <= 0 ? RGB(60, 200, 110) : RGB(70, 80, 130));
    texto(hdc, x + 6, y + 2, nome, RGB(255, 255, 255), f_peq, TA_LEFT | TA_TOP);
}

static void desenhar_hud(HDC hdc)
{
    char buf[100];
    int i, w, seg, mult, x, y;
    float vabs;
    POINT p[4];

    /* barra de vida e vidas */
    retangulo(hdc, 16, 16, 204, 22, RGB(10, 10, 10));
    w = jog.hp * 200 / HP_MAX;
    if (w < 0) w = 0;
    retangulo(hdc, 18, 18, w, 18, jog.hp > 30 ? RGB(190, 30, 40) : RGB(255, 90, 40));
    texto(hdc, 22, 19, "VIDA", RGB(255, 255, 255), f_peq, TA_LEFT | TA_TOP);
    for (i = 0; i < jog.vidas; i++) {
        x = 30 + i * 26; y = 56;
        p[0].x = x;     p[0].y = y - 9;
        p[1].x = x + 9; p[1].y = y;
        p[2].x = x;     p[2].y = y + 9;
        p[3].x = x - 9; p[3].y = y;
        poligono(hdc, p, 4, RGB(220, 30, 60));
    }

    sprintf(buf, "XP %d", xp);
    texto_sombra(hdc, 16, 72, buf, RGB(255, 255, 255), f_media, TA_LEFT | TA_TOP);
    if (combo > 1) {
        sprintf(buf, "COMBO x%d", combo);
        texto_sombra(hdc, 16, 98, buf, RGB(255, 225, 40), f_grande, TA_LEFT | TA_TOP);
        retangulo(hdc, 16, 142, combo_timer * 200 / COMBO_TEMPO, 5, RGB(255, 225, 40));
    }

    /* topo: loucos e tempo */
    sprintf(buf, "LOUCOS %d / %d", loucos_capt, loucos_total);
    texto_sombra(hdc, LARGURA / 2, 10, buf, RGB(255, 225, 40), f_grande, TA_CENTER | TA_TOP);
    seg = tempo_restante / 60;
    if (seg < 0) seg = 0;
    sprintf(buf, "%02d:%02d", seg / 60, seg % 60);
    texto_sombra(hdc, LARGURA / 2, 54, buf, seg < 30 ? RGB(255, 60, 60) : RGB(255, 255, 255),
                 f_media, TA_CENTER | TA_TOP);

    if (msg_timer > 0)
        texto_sombra(hdc, LARGURA / 2, 96, mensagem, RGB(255, 60, 110), f_grande, TA_CENTER | TA_TOP);

    /* habilidades */
    barra_hab(hdc, 16, ALTURA - 100, "E  ALMA", jog.cd_alma, ALMA_RECARGA);
    barra_hab(hdc, 16, ALTURA - 74, "F  PARRY", jog.cd_parry, PARRY_RECARGA);
    barra_hab(hdc, 16, ALTURA - 48, "SHIFT  INVISIVEL", jog.cd_invis, INVIS_RECARGA);

    /* municao ou velocimetro */
    if (jog.no_carro) {
        vabs = (float)fabs(carro.vel);
        mult = mult_vel(vabs);
        sprintf(buf, "%d km/h", (int)vabs);
        texto_sombra(hdc, LARGURA - 20, ALTURA - 100, buf, RGB(255, 255, 255), f_grande, TA_RIGHT | TA_TOP);
        sprintf(buf, "XP x%d", mult);
        texto_sombra(hdc, LARGURA - 20, ALTURA - 60, buf,
                     mult >= 10 ? RGB(255, 60, 60) : RGB(255, 225, 40), f_grande, TA_RIGHT | TA_TOP);
    } else {
        for (i = 0; i < 2; i++) {
            retangulo(hdc, LARGURA - 70 + i * 28, ALTURA - 70, 20, 40, RGB(10, 10, 10));
            if (i < jog.cartuchos) {
                retangulo(hdc, LARGURA - 68 + i * 28, ALTURA - 68, 16, 26, RGB(200, 40, 40));
                retangulo(hdc, LARGURA - 68 + i * 28, ALTURA - 42, 16, 10, RGB(200, 160, 60));
            }
        }
        if (jog.recarga > 0)
            texto_sombra(hdc, LARGURA - 20, ALTURA - 100, "RECARREGANDO", RGB(255, 200, 80), f_media, TA_RIGHT | TA_TOP);
    }

    texto(hdc, LARGURA / 2, ALTURA - 20,
          "WASD mover | MOUSE mira/atira | E alma | F parry | SHIFT invisivel | Q carro | P pausa",
          RGB(170, 170, 170), f_peq, TA_CENTER | TA_TOP);

    /* mira */
    contorno(hdc, mouse_x, mouse_y, 8, RGB(255, 80, 80), 1);
    linha(hdc, mouse_x - 14, mouse_y, mouse_x - 4, mouse_y, 1, RGB(255, 80, 80));
    linha(hdc, mouse_x + 4, mouse_y, mouse_x + 14, mouse_y, 1, RGB(255, 80, 80));
    linha(hdc, mouse_x, mouse_y - 14, mouse_x, mouse_y - 4, 1, RGB(255, 80, 80));
    linha(hdc, mouse_x, mouse_y + 4, mouse_x, mouse_y + 14, 1, RGB(255, 80, 80));
}

/* Painel central para pausa, game over e vitoria */
static void painel(HDC hdc, const char *titulo, COLORREF cor, const char *l1, const char *l2, const char *l3, const char *l4)
{
    retangulo(hdc, 110, ALTURA / 2 - 120, LARGURA - 220, 240, cor);
    retangulo(hdc, 114, ALTURA / 2 - 116, LARGURA - 228, 232, RGB(12, 6, 14));
    texto_sombra(hdc, LARGURA / 2, ALTURA / 2 - 108, titulo, cor, f_titulo, TA_CENTER | TA_TOP);
    texto(hdc, LARGURA / 2, ALTURA / 2 - 5, l1, RGB(255, 255, 255), f_media, TA_CENTER | TA_TOP);
    texto(hdc, LARGURA / 2, ALTURA / 2 + 25, l2, RGB(255, 225, 40), f_media, TA_CENTER | TA_TOP);
    texto(hdc, LARGURA / 2, ALTURA / 2 + 55, l3, RGB(200, 200, 200), f_peq, TA_CENTER | TA_TOP);
    texto(hdc, LARGURA / 2, ALTURA / 2 + 85, l4, RGB(255, 255, 255), f_media, TA_CENTER | TA_TOP);
}

static void desenhar_menu(HDC hdc)
{
    int k, off, y;
    POINT p[4];
    char buf[100];
    COLORREF c1, c2;
    int pisca = (frame / 15) % 2;

    retangulo(hdc, 0, 0, LARGURA, ALTURA, RGB(14, 0, 24));
    off = (frame * 2) % 80;
    for (k = -4; k < 16; k++) { /* listras neon em movimento */
        p[0].x = k * 80 + off;       p[0].y = 0;
        p[1].x = k * 80 + off + 40;  p[1].y = 0;
        p[2].x = k * 80 + off - 160; p[2].y = ALTURA;
        p[3].x = k * 80 + off - 200; p[3].y = ALTURA;
        poligono(hdc, p, 4, RGB(48, 0, 78));
    }

    c1 = pisca ? RGB(255, 40, 150) : RGB(40, 230, 255);
    c2 = pisca ? RGB(40, 230, 255) : RGB(255, 40, 150);
    texto(hdc, LARGURA / 2 + 5, 65, "CASA VERDE", c2, f_titulo, TA_CENTER | TA_TOP);
    texto(hdc, LARGURA / 2, 60, "CASA VERDE", c1, f_titulo, TA_CENTER | TA_TOP);
    texto_sombra(hdc, LARGURA / 2, 170, "A CACADA", RGB(255, 225, 40), f_grande, TA_CENTER | TA_TOP);
    texto_sombra(hdc, LARGURA / 2, 218, "baseado em O Alienista, de Machado de Assis",
                 RGB(255, 255, 255), f_media, TA_CENTER | TA_TOP);

    for (k = 0; k < 4; k++) {
        y = 280 + k * 46;
        sprintf(buf, "%s%d - %s  (%d loucos)", (k == selecao) ? "> " : "   ", k + 1, nome_dif[k], 5 + extra_loucos[k]);
        texto_sombra(hdc, LARGURA / 2, y, buf,
                     (k == selecao) ? RGB(255, 225, 40) : RGB(190, 190, 200), f_grande, TA_CENTER | TA_TOP);
    }

    texto(hdc, LARGURA / 2, 484, "SETAS + ENTER, ou aperte 1 a 4  |  ESC sai", RGB(255, 255, 255), f_media, TA_CENTER | TA_TOP);
    texto(hdc, LARGURA / 2, 524, "WASD mover | MOUSE mira e atira | E suga almas | F parry | SHIFT invisivel | Q carro | P pausa",
          RGB(200, 200, 210), f_peq, TA_CENTER | TA_TOP);
    texto(hdc, LARGURA / 2, 548, "Capture todos os loucos antes do tempo acabar. Atropele em alta velocidade para ganhar mais XP!",
          RGB(200, 200, 210), f_peq, TA_CENTER | TA_TOP);
}

/* Desenha um quadro completo */
static void desenhar_jogo(HDC hdc)
{
    int i, sx, sy, r;
    char buf[100], buf2[100];

    if (estado == ESTADO_MENU) {
        desenhar_menu(hdc);
        return;
    }

    off_x = (int)cam_x + (tremor > 0 ? aleat_int(-tremor / 2, tremor / 2) : 0);
    off_y = (int)cam_y + (tremor > 0 ? aleat_int(-tremor / 2, tremor / 2) : 0);

    desenhar_mundo(hdc);
    desenhar_carro(hdc);
    for (i = 0; i < MAX_INIM; i++)
        if (inim[i].vivo) desenhar_inimigo(hdc, i);
    desenhar_jogador(hdc);

    /* aneis da sucao de almas */
    if (jog.sugando > 0) {
        sx = (int)(jog.x - off_x); sy = (int)(jog.y - off_y);
        r = 20 + (ALMA_RAIO - 20) * jog.sugando / ALMA_DURACAO;
        contorno(hdc, sx, sy, r, RGB(110, 255, 255), 2);
        contorno(hdc, sx, sy, r / 2, RGB(190, 120, 255), 2);
    }

    desenhar_projeteis(hdc);
    desenhar_particulas(hdc);
    desenhar_radar(hdc);
    desenhar_hud(hdc);

    sprintf(buf, "XP: %d   Kills: %d   Maior combo: %d", xp, kills, maior_combo);
    if (estado == ESTADO_PAUSA) {
        painel(hdc, "PAUSADO", RGB(255, 225, 40), "Respire fundo, doutor.", "", "P - Continuar", "ESC - Sair");
    } else if (estado == ESTADO_GAMEOVER) {
        painel(hdc, "GAME OVER", RGB(220, 30, 50), motivo, buf, "Os loucos venceram desta vez...",
               "R - Reiniciar    M - Menu    ESC - Sair");
    } else if (estado == ESTADO_VITORIA) {
        sprintf(buf2, "XP final: %d   Kills: %d   Maior combo: %d", xp, kills, maior_combo);
        painel(hdc, "VOCE VENCEU!", RGB(80, 255, 140), motivo, buf2,
               "Mas quem era o verdadeiro louco?",
               "R - Reiniciar    M - Menu    ESC - Sair");
    }
}

/* ---------------- TECLADO ---------------- */

static void tecla_apertada(int tecla)
{
    if (estado == ESTADO_MENU) {
        if (tecla == VK_UP || tecla == 'W') selecao = (selecao + 3) % 4;
        else if (tecla == VK_DOWN || tecla == 'S') selecao = (selecao + 1) % 4;
        else if (tecla >= '1' && tecla <= '4') {
            selecao = tecla - '1';
            dificuldade = selecao;
            iniciar_jogo();
        } else if (tecla == VK_RETURN || tecla == VK_SPACE) {
            dificuldade = selecao;
            iniciar_jogo();
        }
    } else if (estado == ESTADO_JOGANDO) {
        if (tecla == 'E') usar_alma();
        else if (tecla == 'F') usar_parry();
        else if (tecla == VK_SHIFT) usar_invis();
        else if (tecla == 'Q') alternar_carro();
        else if (tecla == 'P') estado = ESTADO_PAUSA;
    } else if (estado == ESTADO_PAUSA) {
        if (tecla == 'P') estado = ESTADO_JOGANDO;
    } else {
        if (tecla == 'R') iniciar_jogo();
        else if (tecla == 'M') estado = ESTADO_MENU;
    }
}

/* ---------------- JANELA ---------------- */

static LRESULT CALLBACK ProcJanela(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    PAINTSTRUCT ps;
    HDC hdc, dc;

    switch (msg) {
        case WM_CREATE:
            /* cria o buffer para o duplo buffer (tela nao pisca) */
            dc = GetDC(hwnd);
            hdc_buffer = CreateCompatibleDC(dc);
            bmp_buffer = CreateCompatibleBitmap(dc, LARGURA, ALTURA);
            bmp_antigo = (HBITMAP)SelectObject(hdc_buffer, bmp_buffer);
            ReleaseDC(hwnd, dc);
            f_titulo = criar_fonte(96, "Impact");
            f_grande = criar_fonte(34, "Impact");
            f_media = criar_fonte(20, "Arial");
            f_peq = criar_fonte(14, "Arial");
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            desenhar_jogo(hdc_buffer);
            BitBlt(hdc, 0, 0, LARGURA, ALTURA, hdc_buffer, 0, 0, SRCCOPY);
            EndPaint(hwnd, &ps);
            return 0;

        case WM_KEYDOWN:
            if (wp < 256) teclas[wp] = 1;
            if (wp == VK_ESCAPE) {
                DestroyWindow(hwnd);
                return 0;
            }
            if (!(lp & 0x40000000)) tecla_apertada((int)wp); /* so na primeira vez */
            return 0;

        case WM_KEYUP:
            if (wp < 256) teclas[wp] = 0;
            return 0;

        case WM_MOUSEMOVE:
            mouse_x = (short)LOWORD(lp);
            mouse_y = (short)HIWORD(lp);
            return 0;

        case WM_LBUTTONDOWN:
            mouse_botao = 1;
            mouse_x = (short)LOWORD(lp);
            mouse_y = (short)HIWORD(lp);
            return 0;

        case WM_LBUTTONUP:
            mouse_botao = 0;
            return 0;

        case WM_KILLFOCUS:
            memset(teclas, 0, sizeof(teclas));
            mouse_botao = 0;
            return 0;

        case WM_DESTROY:
            /* libera os objetos GDI */
            SelectObject(hdc_buffer, bmp_antigo);
            DeleteObject(bmp_buffer);
            DeleteDC(hdc_buffer);
            DeleteObject(f_titulo);
            DeleteObject(f_grande);
            DeleteObject(f_media);
            DeleteObject(f_peq);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int nShow)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    RECT r;
    LARGE_INTEGER freq, ultimo, agora;
    double acumulado = 0, passo = 1.0 / 60.0;
    int rodando = 1, passos, jw, jh, sw, sh;
    DWORD estilo = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX; /* tamanho fixo */

    srand((unsigned)time(NULL));
    memset(&msg, 0, sizeof(msg));
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = ProcJanela;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS);
    wc.lpszClassName = "CasaVerdeJogo";
    if (!RegisterClassA(&wc)) return 0;

    /* ajusta o tamanho para a area de desenho ficar 960x640 e centraliza */
    r.left = 0; r.top = 0; r.right = LARGURA; r.bottom = ALTURA;
    AdjustWindowRect(&r, estilo, FALSE);
    jw = r.right - r.left;
    jh = r.bottom - r.top;
    sw = GetSystemMetrics(SM_CXSCREEN);
    sh = GetSystemMetrics(SM_CYSCREEN);

    hwnd = CreateWindowA("CasaVerdeJogo", "Casa Verde - A Cacada", estilo,
                         (sw - jw) / 2, (sh - jh) / 2, jw, jh, NULL, NULL, hInst, NULL);
    if (!hwnd) return 0;
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    /* laco do jogo: 60 atualizacoes por segundo */
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&ultimo);
    while (rodando) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) rodando = 0;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!rodando) break;

        QueryPerformanceCounter(&agora);
        acumulado += (double)(agora.QuadPart - ultimo.QuadPart) / (double)freq.QuadPart;
        ultimo = agora;
        if (acumulado > 0.1) acumulado = 0.1;

        passos = 0;
        while (acumulado >= passo && passos < 3) {
            atualizar_jogo();
            acumulado -= passo;
            passos++;
        }
        if (passos > 0) {
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        } else {
            Sleep(1);
        }
    }
    return (int)msg.wParam;
}