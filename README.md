# Sistema de Streaming de Vídeos (C – arquivos binários)
Sistema de streaming de vídeos em C que manipula dois arquivos binários (vídeos e usuários) com relacionamento entre eles: cadastro, leitura, atualização, remoção e relatórios.

**Entrega [Individual] :**

| Nome | RA |
|------|----|
| Letícia de Carvalho Minucelli | 00000865761 |

## Descrição
Programa em C que manipula dois arquivos binários:

- `videos.bin` – registros `Video` (id, título, gênero, duração, ano)
- `usuarios.bin` – registros `Usuario` (id, nome, e-mail, qtd. de favoritos, **vetor de IDs de vídeos favoritados**)

O relacionamento é feito pelo vetor `favoritos[]` do usuário, que guarda IDs de vídeos.

## Funcionalidades
- Cadastro de vídeos e usuários (ID gerado automaticamente)
- Leitura de um registro por ID e listagem de todos
- Atualização de vídeos e usuários (inclusive adicionar/remover favoritos)
- Remoção via arquivo temporário (copia tudo, menos o removido; apaga o antigo e renomeia o novo)
- Ao remover um vídeo, ele também sai dos favoritos de todos os usuários
- Relatórios: usuários com os **títulos** dos favoritos e vídeos com os **nomes** de quem os favoritou
