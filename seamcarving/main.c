#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings

#ifdef WIN32
#include <windows.h> // Apenas para Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>   // Funções da OpenGL
#include <GL/glu.h>  // Funções da GLU
#include <GL/glut.h> // Funções da FreeGLUT
#endif

// SOIL é a biblioteca para leitura das imagens
#include <SOIL.h>

// Um pixel RGB (24 bits)
typedef struct
{
    unsigned char r, g, b;
} RGB8;

// Uma imagem RGB
typedef struct
{
    int width, height;
    RGB8 *img;
} Img;

// Protótipos
void load(char *name, Img *pic);
void uploadTexture();
void seamcarve(int targetWidth);                   // executa o algoritmo
void freemem();                                    // limpa memória (caso tenha alocado dinamicamente)
signed long long calc_pix_e(RGB8 p1, RGB8 p2);     //calculo de energia do pixel
void removepixel(RGB8 *linha, int col, int width); //exclui 1 pixel da linha da imagem
// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);
void arrow_keys(int a_keys, int x, int y);

// Largura e altura da janela
int width, height;

// Largura desejada (selecionável)
int targetW;

// Identificadores de textura
GLuint tex[3];

// As 3 imagens
Img pic[3];
Img *source;
Img *mask;
Img *target;

// Imagem selecionada (0,1,2)
int sel;
//variaveis usadas na funcao seamcarve
RGB8 tgt_msk[600][600];             //mascara ajustada ao tamanho da imagem alvo
signed long long wrk_mci[600][600]; //matriz de energia individual de cada pixel
signed long long wrk_mca[600][600]; //matriz de energia acumulada de cada pixel
//nao foi utilizado alocacao dinamica,
//a fim de poder ver facilmente no vsCode os valores durante a depuracao

// Carrega uma imagem para a struct Img
void load(char *name, Img *pic)
{
    int chan;
    pic->img = (RGB8 *)SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if (!pic->img)
    {
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

// Implemente AQUI o seu algoritmo

//calculo de energia do pixel
signed long long calc_pix_e(RGB8 p1, RGB8 p2)
{
    signed long long c;
    signed long long e = 0;
    //Faz o cálculo do gradiente a partir dos pixels vizinhos

    c = p1.r;     //busca o componente do primeiro vizinho do pixel
    c = c - p2.r; //subtrai a componente do outro vizinho
    c = c * c;    //eleva ao quadrado
    e = e + c;    //soma a energia

    //faz o mesmo para todos os componentes de cor
    c = p1.g;
    c = c - p2.g;
    c = c * c;
    e = e + c;

    c = p1.b;
    c = c - p2.b;
    c = c * c;
    e = e + c;

    return e;
}

//remove um pixel e faz o shift para esquerda da linha de pixels restante

void removepixel(RGB8 *linha, int col, int width)
{
    for (int x = col; x < width - 1; x++)
    {
        linha[x] = linha[x + 1]; //faz o shift
    }
    linha[width - 1].r = linha[width - 1].g = linha[width - 1].b = 0; // cria o ultimo pixel em preto
}

void seamcarve(int targetWidth)
{
    // Aplica o algoritmo e gera a saida em target->img...

    RGB8(*src_img)
    [target->width] = (RGB8(*)[target->width])source->img; //imagem original
    RGB8(*src_msk)
    [target->width] = (RGB8(*)[target->width])mask->img; //mascara da imagem original
    RGB8(*tgt_img)
    [target->width] = (RGB8(*)[target->width])target->img; //imagem resultante do algoritmo

    int current_width = target->width; // inicia tamanho total
    //carrega as imagens de trabalho
    for (int y = 0; y < target->height; y++)
    {
        for (int x = 0; x < current_width; x++)
        {
            tgt_img[y][x] = src_img[y][x]; //imagem
            tgt_msk[y][x] = src_msk[y][x]; //mascara
        }
    }

    while (current_width > targetWidth)
    { //loop ate que chegue a largura desejada da imagem

        //calculo de energia de cada pixel
        for (int y = 0; y < target->height; y++)
        {
            for (int x = 0; x < current_width; x++)
            {
                //coordenadas dos pixels vizinhos
                int xr;
                int xl;
                int yu;
                int yd;

                signed long long e = 0; //energia do pixel

                //calcula as coordenadas dos pixels vizinhos, levando em conta as bordas da imagem
                xr = x + 1;
                if (xr >= current_width) //borda direita
                {
                    xr = 0;
                }
                xl = x - 1;
                if (x == 0) //borda esquerda
                {
                    xl = current_width - 1;
                }
                yd = y + 1;
                if (yd >= target->height) //borda inferior
                {
                    yd = 0;
                }
                yu = y - 1;
                if (y == 0) //borda superior
                {
                    yu = target->height - 1;
                }

                e = calc_pix_e(tgt_img[y][xr], tgt_img[y][xl]);     //obtem a energia dos vizinhos x
                e = e + calc_pix_e(tgt_img[yu][x], tgt_img[yd][x]); //adiciona a energia dos vizinhos y

                //verifica se o pixel foi afetado pela mascara
                if ((tgt_msk[y][x].r - tgt_msk[y][x].g) > 10) //o pixel da mascara e vermelho
                {
                    e = -40000000; //atribui um valor extremamente baixo
                }
                if ((tgt_msk[y][x].g - tgt_msk[y][x].r) > 10) //o pixel da mascara e verde
                {
                    e = 40000000; //atribui um valor extremamente alto
                }
                wrk_mci[y][x] = e; //armazena o resultado na matriz de energia
            }
        }

        // calculo da matriz de energia acumulada
        for (int y = 0; y < target->height; y++)
        {
            for (int x = 0; x < current_width; x++)
            {
                if (y == 0) //a primeira linha recebe a energia do proprio  pixel
                {
                    wrk_mca[y][x] = wrk_mci[y][x];
                }
                else
                { //as demais linhas recebem o custo acumulado

                    int xl, xr;          //coordenadas dos vizinhos
                    signed long long ca; //custo acumulado

                    //calcula as coordenadas dos vizinhos
                    xr = x + 1;
                    if (xr >= current_width) //borda da direita
                    {
                        xr = x;
                    }
                    xl = x - 1;
                    if (x == 0) //borda da esquerda
                    {
                        xl = x;
                    }

                    ca = wrk_mca[y - 1][x];      // tenta primeiro com o vizinho central
                    if (ca > wrk_mca[y - 1][xl]) // se o vizinho da esquerda e menor custo
                    {
                        ca = wrk_mca[y - 1][xl]; // substitui o custo
                    }
                    if (ca > wrk_mca[y - 1][xr]) // se o vizinho da direita e menor custo
                    {
                        ca = wrk_mca[y - 1][xr]; // substitui o custo
                    }

                    wrk_mca[y][x] = ca + wrk_mci[y][x]; // acumula a energia do pixel atual
                }
            }
        }
        //encontrar o seam com menor soma acumulada de energia
        signed long long min_ca_val;
        int min_ca_x;

        min_ca_val = wrk_mca[target->height - 1][0]; //valor inicial
        min_ca_x = 0;                                //valor inicial

        //basta fazer o caminho inverso, isto é, de baixo para cima.
        for (int y = target->height - 1; y >= 0; y--)
        {
            if (y == target->height - 1) //ultima linha da matriz
            {
                for (int x = 0; x < current_width; x++) //encontrar o menor custo acumulado
                {
                    if (min_ca_val > wrk_mca[y][x]) //se o valor atual e maior do que o do pixel
                    {
                        min_ca_val = wrk_mca[y][x]; //substitui pelo menor valor
                        min_ca_x = x;               //e salva a coordenada do pixel de menor custo
                    }
                }
                //remove o pixel de menor custo nessa linha
                removepixel(tgt_img[y], min_ca_x, target->width); //remove da imagem
                removepixel(tgt_msk[y], min_ca_x, target->width); //remove da mascara
            }
            else
            {
                // nas demais linhas encontra o caminho reverso de menor custo acumulado

                int xl, xr; //coordenadas dos vizinhos

                //calcula as coordenadas dos vizinhos
                xr = min_ca_x + 1;
                if (xr >= current_width) //borda da direita
                {
                    xr = min_ca_x;
                }
                xl = min_ca_x - 1;
                if (min_ca_x == 0) //borda da esquerda
                {
                    xl = min_ca_x;
                }

                min_ca_val = wrk_mca[y][min_ca_x]; // tenta primeiro com vizinho do meio
                if (min_ca_val > wrk_mca[y][xl])   // se o vizinho da esquerda e menor custo
                {
                    min_ca_val = wrk_mca[y][xl]; //substitui o custo
                    min_ca_x = xl;               //e salva a coordenada do pixel de menor custo
                }
                if (min_ca_val > wrk_mca[y][xr]) // se o vizinho da direita e menor custo
                {
                    min_ca_val = wrk_mca[y][xr]; //substitui o custo
                    min_ca_x = xr;               //e salva a coordenada do pixel de menor custo
                }
                //remove o pixel de menor custo nessa linha
                removepixel(tgt_img[y], min_ca_x, target->width); //remove da imagem
                removepixel(tgt_msk[y], min_ca_x, target->width); //remove da mascara
            }
        }

        current_width--; //a imagem esta menor em 1 pixel
    }

    // Chame uploadTexture a cada vez que mudar
    // a imagem (pic[2])
    uploadTexture();
    glutPostRedisplay();
}

void freemem()
{
    // Libera a memória ocupada pelas 3 imagens
    free(pic[0].img);
    free(pic[1].img);
    free(pic[2].img);
}

/********************************************************************
 * 
 *  VOCÊ NÃO DEVE ALTERAR NADA NO PROGRAMA A PARTIR DESTE PONTO!
 *
 ********************************************************************/
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("seamcarving [origem] [mascara]\n");
        printf("Origem é a imagem original, mascara é a máscara desejada\n");
        exit(1);
    }
    glutInit(&argc, argv);

    // Define do modo de operacao da GLUT
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // pic[0] -> imagem original
    // pic[1] -> máscara desejada
    // pic[2] -> resultado do algoritmo

    // Carrega as duas imagens
    load(argv[1], &pic[0]);
    load(argv[2], &pic[1]);

    if (pic[0].width != pic[1].width || pic[0].height != pic[1].height)
    {
        printf("Imagem e máscara com dimensões diferentes!\n");
        exit(1);
    }

    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem original (1)
    pic[2].width = pic[1].width;
    pic[2].height = pic[1].height;

    // Ponteiros para as structs das imagens, para facilitar
    source = &pic[0];
    mask = &pic[1];
    target = &pic[2];

    // Largura desejada inicialmente é a largura da janela
    targetW = target->width;

    // Especifica o tamanho inicial em pixels da janela GLUT
    glutInitWindowSize(width, height);

    // Cria a janela passando como argumento o titulo da mesma
    glutCreateWindow("Seam Carving");

    // Registra a funcao callback de redesenho da janela de visualizacao
    glutDisplayFunc(draw);

    // Registra a funcao callback para tratamento das teclas ASCII
    glutKeyboardFunc(keyboard);

    // Registra a funcao callback para tratamento das setas
    glutSpecialFunc(arrow_keys);

    // Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char *)pic[0].img, pic[0].width, pic[0].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char *)pic[1].img, pic[1].width, pic[1].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    printf("Máscara : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
    sel = 0; // pic1

    // Define a janela de visualizacao 2D
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0.0, width, height, 0.0);
    glMatrixMode(GL_MODELVIEW);

    // Aloca memória para a imagem de saída
    pic[2].img = malloc(pic[1].width * pic[1].height * 3); // W x H x 3 bytes (RGB)
    // Pinta a imagem resultante de preto!
    memset(pic[2].img, 0, width * height * 3);

    // Cria textura para a imagem de saída
    tex[2] = SOIL_create_OGL_texture((unsigned char *)pic[2].img, pic[2].width, pic[2].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Entra no loop de eventos, não retorna
    glutMainLoop();
}

// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        // ESC: libera memória e finaliza
        freemem();
        exit(1);
    }
    if (key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, máscara e resultado)
        sel = key - '1';
    if (key == 's')
    {
        seamcarve(targetW);
    }
    glutPostRedisplay();
}

void arrow_keys(int a_keys, int x, int y)
{
    switch (a_keys)
    {
    case GLUT_KEY_RIGHT:
        if (targetW <= pic[2].width - 10)
            targetW += 10;
        seamcarve(targetW);
        break;
    case GLUT_KEY_LEFT:
        if (targetW > 10)
            targetW -= 10;
        seamcarve(targetW);
        break;
    default:
        break;
    }
}
// Faz upload da imagem para a textura,
// de forma a exibi-la na tela
void uploadTexture()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 target->width, target->height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, target->img);
    glDisable(GL_TEXTURE_2D);
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Preto
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255); // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0, 0);
    glVertex2f(0, 0);

    glTexCoord2f(1, 0);
    glVertex2f(pic[sel].width, 0);

    glTexCoord2f(1, 1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0, 1);
    glVertex2f(0, pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
