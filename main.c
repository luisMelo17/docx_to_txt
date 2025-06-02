#include <stdio.h>
#include <zip.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>

void remover_tags_xml_com_imagens(const char *xml, FILE *saida) {
    int dentro_de_tag = 0;
    int pos_tag = 0;
    char buffer_tag[1024];

    for (size_t i = 0; xml[i]; i++) {
        if (xml[i] == '<') {
            dentro_de_tag = 1;
            pos_tag = 0;
            buffer_tag[0] = '\0';
        } else if (xml[i] == '>') {
            dentro_de_tag = 0;
            buffer_tag[pos_tag] = '\0';

            // Verifica se a tag tem referência a imagem
            if (strstr(buffer_tag, "drawing") || strstr(buffer_tag, "blip")) {
                char *rid = strstr(buffer_tag, "r:embed=");
                if (rid) {
                    fprintf(saida, "[IMAGEM]");
                }
            }
        } else if (dentro_de_tag) {
            if (pos_tag < sizeof(buffer_tag) - 1)
                buffer_tag[pos_tag++] = xml[i];
        } else {
            fputc(xml[i], saida);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s entrada.docx saida.txt\n", argv[0]);
        return 1;
    }

    int erro = 0;
    zip_t *zip = zip_open(argv[1], ZIP_RDONLY, &erro);
    if (!zip) {
        fprintf(stderr, "Falha ao abrir o arquivo .docx\n");
        return 1;
    }

    zip_file_t *arquivo_xml = zip_fopen(zip, "word/document.xml", 0);
    if (!arquivo_xml) {
        fprintf(stderr, "Falha ao abrir document.xml dentro do .docx\n");
        zip_close(zip);
        return 1;
    }

    zip_stat_t stat_xml;
    zip_stat(zip, "word/document.xml", 0, &stat_xml);

    char *conteudo = malloc(stat_xml.size + 1);
    zip_fread(arquivo_xml, conteudo, stat_xml.size);
    conteudo[stat_xml.size] = '\0';

    FILE *saida = fopen(argv[2], "w");
    if (!saida) {
        fprintf(stderr, "Falha ao criar o arquivo de saída\n");
        free(conteudo);
        zip_fclose(arquivo_xml);
        zip_close(zip);
        return 1;
    }

    remover_tags_xml_com_imagens(conteudo, saida);

    // Lista imagens embutidas
    fprintf(saida, "\n\nImagens embutidas:\n");
    zip_int64_t total_arquivos = zip_get_num_entries(zip, 0);
    for (zip_uint64_t i = 0; i < total_arquivos; i++) {
        const char *nome = zip_get_name(zip, i, 0);
        if (nome && strncmp(nome, "word/media/", 11) == 0) {
            fprintf(saida, "- %s\n", nome + 11);
        }
    }

    fclose(saida);
    free(conteudo);
    zip_fclose(arquivo_xml);
    zip_close(zip);

    return 0;
}

