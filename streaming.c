#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define ARQ_VIDEOS   "videos.bin"
#define ARQ_USUARIOS "usuarios.bin"
#define ARQ_TEMP     "temp.bin"
 
#define TAM_TEXTO    60
#define MAX_FAV      20
 
typedef struct {
    int   id; 
    char  titulo[TAM_TEXTO];
    char  genero[TAM_TEXTO];
    int   duracao_min;
    int   ano;
} Video;
 
typedef struct {
    int   id;
    char  nome[TAM_TEXTO];
    char  email[TAM_TEXTO];
    int   qtd_favoritos;
    int   favoritos[MAX_FAV]; /* vetor de IDs de Video */
} Usuario;
 
/*utilitarios de entrada */
 
static void ler_texto(const char *prompt, char *dest, int tam) {
    printf("%s", prompt);
    if (fgets(dest, tam, stdin) == NULL) { dest[0] = '\0'; return; }
    size_t n = strlen(dest);
    if (n > 0 && dest[n - 1] == '\n') {
        dest[n - 1] = '\0';
    } else {
        int c; 
        while ((c = getchar()) != '\n' && c != EOF);
    }
}
 
static int ler_int(const char *prompt) {
    char buf[64];
    ler_texto(prompt, buf, sizeof buf);
    return atoi(buf);
}
 
/*acesso generico aos arquivos*/
 
/*procura registro pelo ID. retorna a posicao (indice) ou -1 se nao achar. */
static long buscar_registro(const char *arquivo, size_t tam, int id, void *saida) {
    FILE *f = fopen(arquivo, "rb");
    if (!f) return -1;
    char *buf = malloc(tam);
    long pos = 0, achou = -1;
    while (fread(buf, tam, 1, f) == 1) {
        if (*(int *)buf == id) {
            if (saida) memcpy(saida, buf, tam);
            achou = pos;
            break;
        }
        pos++;
    }
    free(buf);
    fclose(f);
    return achou;
}
 
/*sobrescreve o registro na posicao 'pos' (atualizacao)*/
static int atualizar_registro(const char *arquivo, size_t tam, long pos, const void *reg) {
    FILE *f = fopen(arquivo, "r+b");
    if (!f) return 0;
    fseek(f, pos * (long)tam, SEEK_SET);
    int ok = fwrite(reg, tam, 1, f) == 1;
    fclose(f);
    return ok;
}
 
/*screscenta um registro novo ao final do arquivo*/
static int gravar_registro(const char *arquivo, size_t tam, const void *reg) {
    FILE *f = fopen(arquivo, "ab");
    if (!f) return 0;
    int ok = fwrite(reg, tam, 1, f) == 1;
    fclose(f);
    return ok;
}
 
/*proximo ID = maior ID existente + 1*/
static int proximo_id(const char *arquivo, size_t tam) {
    FILE *f = fopen(arquivo, "rb");
    if (!f) return 1;
    char *buf = malloc(tam);
    int maior = 0;
    while (fread(buf, tam, 1, f) == 1)
        if (*(int *)buf > maior) maior = *(int *)buf;
    free(buf);
    fclose(f);
    return maior + 1;
}
 
/*
 *remoção: cria um arquivo novo com todos os registros menos o removido,
 * fecha os dois, apaga o antigo e renomeia o novo para o nome do antigo.
 */
static int remover_registro(const char *arquivo, size_t tam, int id) {
    FILE *antigo = fopen(arquivo, "rb");
    if (!antigo) return 0;
    FILE *novo = fopen(ARQ_TEMP, "wb");
    if (!novo) { fclose(antigo); return 0; }
 
    char *buf = malloc(tam);
    int removido = 0;
    while (fread(buf, tam, 1, antigo) == 1) {
        if (*(int *)buf == id) { removido = 1; continue; }
        fwrite(buf, tam, 1, novo);
    }
    free(buf);
    fclose(antigo);
    fclose(novo);
 
    remove(arquivo);
    rename(ARQ_TEMP, arquivo);
    return removido;
}
 
/*funcoes de apoio ao relacionamento*/
 
static int usuario_tem_favorito(const Usuario *u, int id_video) {
    for (int i = 0; i < u->qtd_favoritos; i++)
        if (u->favoritos[i] == id_video) return 1;
    return 0;
}
 
/*apos remover um video, tira o ID dele dos favoritos de todos os usuarios*/
static void limpar_favorito_em_todos(int id_video) {
    FILE *f = fopen(ARQ_USUARIOS, "r+b");
    if (!f) return;
    Usuario u;
    while (fread(&u, sizeof u, 1, f) == 1) {
        int j = 0, mudou = 0;
        for (int i = 0; i < u.qtd_favoritos; i++) {
            if (u.favoritos[i] == id_video) { mudou = 1; continue; }
            u.favoritos[j++] = u.favoritos[i];
        }
        if (mudou) {
            u.qtd_favoritos = j;
            fseek(f, -(long)sizeof u, SEEK_CUR);
            fwrite(&u, sizeof u, 1, f);
            fseek(f, 0, SEEK_CUR);
        }
    }
    fclose(f);
}
 
/*impressao*/
 
static void imprimir_video(const Video *v) {
    printf("  [ID %d] %s | Genero: %s | %d min | Ano: %d\n",
           v->id, v->titulo, v->genero, v->duracao_min, v->ano);
}
 
/*imprime o usuario mostrando os TITULOS dos videos favoritos (busca em videos.bin) */
static void imprimir_usuario_completo(const Usuario *u) {
    printf("  [ID %d] %s <%s>\n", u->id, u->nome, u->email);
    if (u->qtd_favoritos == 0) {
        printf("      (nenhum favorito)\n");
        return;
    }
    for (int i = 0; i < u->qtd_favoritos; i++) {
        Video v;
        if (buscar_registro(ARQ_VIDEOS, sizeof(Video), u->favoritos[i], &v) >= 0)
            printf("      * %s (%s, %d)\n", v.titulo, v.genero, v.ano);
        else
            printf("      * [video ID %d nao encontrado]\n", u->favoritos[i]);
    }
}
 
/*videos*/
 
static void cadastrar_video(void) {
    Video v;
    memset(&v, 0, sizeof v);
    v.id = proximo_id(ARQ_VIDEOS, sizeof(Video));
    ler_texto("Titulo: ", v.titulo, TAM_TEXTO);
    ler_texto("Genero: ", v.genero, TAM_TEXTO);
    v.duracao_min = ler_int("Duracao (minutos): ");
    v.ano = ler_int("Ano de lancamento: ");
    if (gravar_registro(ARQ_VIDEOS, sizeof v, &v))
        printf("Video cadastrado com ID %d.\n", v.id);
    else
        printf("Erro ao gravar video.\n");
}
 
static void atualizar_video(void) {
    int id = ler_int("ID do video a atualizar: ");
    Video v;
    long pos = buscar_registro(ARQ_VIDEOS, sizeof v, id, &v);
    if (pos < 0) { printf("Video nao encontrado.\n"); return; }
 
    printf("Registro atual:\n");
    imprimir_video(&v);
    printf("(deixe em branco / 0 para manter o valor atual)\n");
 
    char buf[TAM_TEXTO];
    ler_texto("Novo titulo: ", buf, TAM_TEXTO);
    if (buf[0]) strcpy(v.titulo, buf);
    ler_texto("Novo genero: ", buf, TAM_TEXTO);
    if (buf[0]) strcpy(v.genero, buf);
    int n = ler_int("Nova duracao: ");
    if (n > 0) v.duracao_min = n;
    n = ler_int("Novo ano: ");
    if (n > 0) v.ano = n;
 
    if (atualizar_registro(ARQ_VIDEOS, sizeof v, pos, &v))
        printf("Video atualizado.\n");
    else
        printf("Erro ao atualizar.\n");
}
 
static void ler_um_video(void) {
    int id = ler_int("ID do video: ");
    Video v;
    if (buscar_registro(ARQ_VIDEOS, sizeof v, id, &v) < 0)
        printf("Video nao encontrado.\n");
    else
        imprimir_video(&v);
}
 
static void listar_videos(void) {
    FILE *f = fopen(ARQ_VIDEOS, "rb");
    if (!f) { printf("Nenhum video cadastrado.\n"); return; }
    Video v;
    int cont = 0;
    printf("\n--- VIDEOS ---\n");
    while (fread(&v, sizeof v, 1, f) == 1) { imprimir_video(&v); cont++; }
    fclose(f);
    if (cont == 0) printf("  (vazio)\n");
}
 
static void remover_video(void) {
    int id = ler_int("ID do video a remover: ");
    if (remover_registro(ARQ_VIDEOS, sizeof(Video), id)) {
        limpar_favorito_em_todos(id);
        printf("Video removido (e retirado dos favoritos dos usuarios).\n");
    } else {
        printf("Video nao encontrado.\n");
    }
}
 
/*usuarios*/
 
static void cadastrar_usuario(void) {
    Usuario u;
    memset(&u, 0, sizeof u);
    u.id = proximo_id(ARQ_USUARIOS, sizeof(Usuario));
    ler_texto("Nome: ", u.nome, TAM_TEXTO);
    ler_texto("Email: ", u.email, TAM_TEXTO);
 
    printf("Cadastre os IDs dos videos favoritos (0 para parar, max %d).\n", MAX_FAV);
    while (u.qtd_favoritos < MAX_FAV) {
        int idv = ler_int("  ID do video favorito: ");
        if (idv == 0) break;
        if (buscar_registro(ARQ_VIDEOS, sizeof(Video), idv, NULL) < 0)
            printf("  Video %d nao existe.\n", idv);
        else if (usuario_tem_favorito(&u, idv))
            printf("  Video ja esta nos favoritos.\n");
        else
            u.favoritos[u.qtd_favoritos++] = idv;
    }
    if (gravar_registro(ARQ_USUARIOS, sizeof u, &u))
        printf("Usuario cadastrado com ID %d.\n", u.id);
    else
        printf("Erro ao gravar usuario.\n");
}
 
static void atualizar_usuario(void) {
    int id = ler_int("ID do usuario a atualizar: ");
    Usuario u;
    long pos = buscar_registro(ARQ_USUARIOS, sizeof u, id, &u);
    if (pos < 0) { printf("Usuario nao encontrado.\n"); return; }
 
    printf("Registro atual:\n");
    imprimir_usuario_completo(&u);
    printf("\n1 - Alterar nome/email\n2 - Adicionar video favorito\n3 - Remover video favorito\nOpcao: ");
    int op = ler_int("");
 
    if (op == 1) {
        char buf[TAM_TEXTO];
        ler_texto("Novo nome (vazio mantem): ", buf, TAM_TEXTO);
        if (buf[0]) strcpy(u.nome, buf);
        ler_texto("Novo email (vazio mantem): ", buf, TAM_TEXTO);
        if (buf[0]) strcpy(u.email, buf);
    } else if (op == 2) {
        if (u.qtd_favoritos >= MAX_FAV) { printf("Limite de favoritos atingido.\n"); return; }
        int idv = ler_int("ID do video: ");
        if (buscar_registro(ARQ_VIDEOS, sizeof(Video), idv, NULL) < 0) {
            printf("Video nao existe.\n"); return;
        }
        if (usuario_tem_favorito(&u, idv)) { printf("Ja esta nos favoritos.\n"); return; }
        u.favoritos[u.qtd_favoritos++] = idv;
    } else if (op == 3) {
        int idv = ler_int("ID do video a remover dos favoritos: ");
        int j = 0, achou = 0;
        for (int i = 0; i < u.qtd_favoritos; i++) {
            if (u.favoritos[i] == idv) { achou = 1; continue; }
            u.favoritos[j++] = u.favoritos[i];
        }
        if (!achou) { printf("Video nao esta nos favoritos.\n"); return; }
        u.qtd_favoritos = j;
    } else {
        printf("Opcao invalida.\n");
        return;
    }
 
    if (atualizar_registro(ARQ_USUARIOS, sizeof u, pos, &u))
        printf("Usuario atualizado.\n");
    else
        printf("Erro ao atualizar.\n");
}
 
static void ler_um_usuario(void) {
    int id = ler_int("ID do usuario: ");
    Usuario u;
    if (buscar_registro(ARQ_USUARIOS, sizeof u, id, &u) < 0)
        printf("Usuario nao encontrado.\n");
    else
        imprimir_usuario_completo(&u);
}
 
static void listar_usuarios(void) {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    if (!f) { printf("Nenhum usuario cadastrado.\n"); return; }
    Usuario u;
    int cont = 0;
    printf("\n--- USUARIOS ---\n");
    while (fread(&u, sizeof u, 1, f) == 1) { imprimir_usuario_completo(&u); cont++; }
    fclose(f);
    if (cont == 0) printf("  (vazio)\n");
}
 
static void remover_usuario(void) {
    int id = ler_int("ID do usuario a remover: ");
    if (remover_registro(ARQ_USUARIOS, sizeof(Usuario), id))
        printf("Usuario removido.\n");
    else
        printf("Usuario nao encontrado.\n");
}
 
/*relatorios*/
 
/*todos os usuarios com os nomes dos videos favoritados*/
static void relatorio_usuarios_favoritos(void) {
    FILE *f = fopen(ARQ_USUARIOS, "rb");
    if (!f) { printf("Nenhum usuario cadastrado.\n"); return; }
    Usuario u;
    printf("\n===== RELATORIO: USUARIOS E SEUS VIDEOS FAVORITOS =====\n");
    while (fread(&u, sizeof u, 1, f) == 1) {
        imprimir_usuario_completo(&u);
        printf("\n");
    }
    fclose(f);
}
 
/*todos os videos com os nomes dos usuarios que os favoritaram (relacao inversa) */
static void relatorio_videos_fas(void) {
    FILE *fv = fopen(ARQ_VIDEOS, "rb");
    if (!fv) { printf("Nenhum video cadastrado.\n"); return; }
    Video v;
    printf("\n===== RELATORIO: VIDEOS E USUARIOS QUE FAVORITARAM =====\n");
    while (fread(&v, sizeof v, 1, fv) == 1) {
        imprimir_video(&v);
        FILE *fu = fopen(ARQ_USUARIOS, "rb");
        int total = 0;
        if (fu) {
            Usuario u;
            while (fread(&u, sizeof u, 1, fu) == 1) {
                if (usuario_tem_favorito(&u, v.id)) {
                    printf("      - favoritado por: %s\n", u.nome);
                    total++;
                }
            }
            fclose(fu);
        }
        if (total == 0) printf("      (ninguem favoritou)\n");
        else printf("      Total: %d usuario(s)\n", total);
        printf("\n");
    }
    fclose(fv);
}