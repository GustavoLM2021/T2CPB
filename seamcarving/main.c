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
void seamcarve(int targetWidth); // executa o algoritmo
void freemem();                  // limpa memória (caso tenha alocado dinamicamente)

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

    RGB8 tgt_msk[600][600];
    signed long long wrk_mci[600][600];
    signed long long wrk_mca[600][600];


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

//
// Implemente AQUI o seu algoritmo

signed long long calc_pix_e(RGB8 p1, RGB8 p2){
    signed long long c;
    signed long long e=0;
    c = p1.r;
    c = c - p2.r;
    c = c * c;
    e = e + c;

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
void removepixel(RGB8 *linha, int col, int width)
{
    for (int x = col; x < width - 1; x++)
    {
       linha[x] = linha[x + 1];
    }
    linha[width - 1].r = linha[width - 1].g = linha[width - 1].b=0; // ultima coluna em preto   
}


void seamcarve(int targetWidth)
{
    // Aplica o algoritmo e gera a saida em target->img...

    size_t img_size = sizeof(RGB8) * (target->width) * (target->height);
    
    RGB8(*src_img)[target->width] = (RGB8(*)[target->width])source->img;
    RGB8(*src_msk)[target->width] = (RGB8(*)[target->width])mask->img;
    RGB8(*tgt_img)[target->width] = (RGB8(*)[target->width])target->img;
    signed long long  min_ca_val;
    int min_ca_x;
  
    
    int current_width = target->width; // inicia tamanho total
    
    for (int y = 0; y < target->height; y++)
        {
            for (int x = 0; x < current_width; x++)
            {
                tgt_img[y][x] = src_img[y][x];
                tgt_msk[y][x] = src_msk[y][x];
            }
        }

    while (current_width > targetWidth)
    { // ate que chegue ao tamanho desejado

        for (int y = 0; y < target->height; y++)
        {
            for (int x = 0; x < current_width; x++)
            {
                // calcula energia x
                int xr;
                int xl;
                int yu;
                int yd;

                signed long long e = 0;
                signed long long c = 0;
                xr = x + 1;
                if (xr >= current_width)
                {
                    xr = 0;
                }
                xl = x - 1;
                if (x == 0)
                {
                    xl = current_width - 1;
                }
                yd = y + 1;
                if (yd >= target->height)
                {
                    yd = 0;
                }
                yu = y - 1;
                if (y == 0)
                {
                    yu = target->height - 1;
                }

                e = calc_pix_e(tgt_img[y][xr], tgt_img[y][xl]);
                e = e + calc_pix_e(tgt_img[yu][x], tgt_img[yd][x]);

                // energia do pixel
                if ((tgt_msk[y][x].r - tgt_msk[y][x].g)>10)
                {
                    e += -40000000;
                
                }
                if ((tgt_msk[y][x].g - tgt_msk[y][x].r)>50)
                {
                    e += 40000000;
                }
                wrk_mci[y][x] = e;

                
            }
        }
        
        // calculo da matriz de energia acumulada
        for (int y = 0; y < target->height; y++)
        {
            for (int x = 0; x < current_width; x++)
            {
                if (y == 0)
                {
                    wrk_mca[y][x] = wrk_mci[y][x];
                }
                else
                {
                    int xl, xr;
                    signed long long  ca;
                    xr = x + 1;
                    if (xr >= current_width)
                    {
                        xr = x;
                    }
                    xl = x - 1;
                    if (x == 0)
                    {
                        xl = x;
                    }

                    ca = wrk_mca[y - 1][x];      // tenta primeiro com a celula central
                    if (ca > wrk_mca[y - 1][xl]) // se esquerda menor
                    {
                        ca = wrk_mca[y - 1][xl]; // substitui
                    }
                    if (ca > wrk_mca[y - 1][xr]) // se direita menor
                    {
                        ca = wrk_mca[y - 1][xr]; // substitui
                    }

                    wrk_mca[y][x] = ca + wrk_mci[y][x]; // acumula
                }
            }
        }

        min_ca_val = wrk_mca[target->height - 1][0];
        min_ca_x = 0;

        for (int y = target->height - 1; y >= 0; y--)
        {

            if (y == target->height - 1)
            {
                for (int x = 0; x < current_width; x++)
                {
                    if (min_ca_val > wrk_mca[y][x])
                    {
                        min_ca_val = wrk_mca[y][x];
                        min_ca_x = x;
                    }
                }
                removepixel(tgt_img[y], min_ca_x, target->width);
                removepixel(tgt_msk[y], min_ca_x, target->width);
            }
            else
            {
                // nas demais linhas basta testar os vizinhos

                int xl, xr;
                xr = min_ca_x + 1;
                if (xr >= current_width)
                {
                    xr = min_ca_x;
                }
                xl = min_ca_x - 1;
                if (min_ca_x == 0)
                {
                    xl = min_ca_x;
                }

                min_ca_val = wrk_mca[y][min_ca_x]; // tenta primeiro com a do meio
                if (min_ca_val > wrk_mca[y][xl])
                {
                    min_ca_val = wrk_mca[y][xl];
                    min_ca_x = xl;
                }
                if (min_ca_val > wrk_mca[y][xr])
                {
                    min_ca_val = wrk_mca[y][xr];
                    min_ca_x = xr;
                }

                removepixel(tgt_img[y], min_ca_x, target->width);
                removepixel(tgt_msk[y], min_ca_x, target->width);
            }
        }

        current_width--;
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
