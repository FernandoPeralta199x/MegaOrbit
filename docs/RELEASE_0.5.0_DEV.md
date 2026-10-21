# MegaOrbit 0.5.0-dev — teste jogável e progresso de emulação

## Resultado verificado por CI

- 18/18 testes de instruções e barramento 68000 aprovados.
- Testes extras de Z80 parcial, PSG, FM aproximada e VDP parcial aprovados.
- Teste automatizado de cartucho 68000 original, com leitura do D-Pad virtual, aprovado.
- Testes de navegação, pausa, slots e verificação de snapshots em RAM aprovados.
- Compilação cruzada SH-2 via libyaul; validação estrutural BIN/CUE de Saturn aprovada.

**Importante:** nenhuma dessas verificações substitui rodar a nova imagem com vídeo e áudio num emulador Sega Saturn ou no hardware físico.

## Jogo de teste ORBIT PADDLE

O código-fonte original está em `core/homebrew.c`. O ROM de 1 KiB emulada contém um programa real em código de máquina Motorola 68000. O laço lê A10003 (controle Mega Drive; bits LEFT/RIGHT ativos em nível baixo), aplica ANDI/BNE e modifica D1 com SUBQ.W e ADDQ.W. O Saturn usa D1 para posicionar o cursor do jogo em uma pista ASCII e administra o alvo e a pontuação na interface nativa.

Isso é um **jogo demonstrativo híbrido** — a lógica de movimento vem do 68000 emulado, mas o desenho e o alvo são geridos pelo aplicativo Saturn, não pelo VDP do Mega Drive. Serve para validar CPU + controle + menu em condições interativas; **não equivale a rodar um título comercial nem a uma implementação completa do hardware Mega Drive**.

No menu, entre em `Selecionar Jogo > Orbit Paddle (Homebrew)`. Use esquerda/direita para mover `O` até o asterisco `*`. O contador de pontos deve avançar. Abra o menu de pausa com `L + R + Start`.

## Menu Saroo-like

- Backdrop criado programaticamente no **VDP2 NBG1**, em bitmap 512x256 / 8bpp, recorte visível 320x224.
- Fonte de debug libyaul por cima, em **VDP2 NBG3**, usando bank 3. O bitmap ocupa bank 0, sem sobrepor os dados de glifos.
- Gradiente espacial simples, grade neon, bordas azuis, destaque da linha selecionada, lista de jogos e detalhes.
- Fundo escurecido durante a pausa, sem captura do framebuffer do Mega Drive. Layout é inspirado na organização do Saroo, **não usa firmware nem imagens proprietárias do Saroo**.

## Menu in-game

```text
============= MEGA ORBIT =============
             JOGO PAUSADO

   > CONTINUAR
     SALVAR ESTADO
     CARREGAR ESTADO
     SLOT [ 01 ]
     RESETAR TESTE
     VOLTAR AO MENU
======================================
```

L+R+Start abre/fecha durante Orbit Paddle e telas de diagnóstico; D-Pad navega, A/C confirma, B volta.

### O que o salvamento faz hoje

Os comandos salvar/carregar **funcionam em RAM temporária para o mini-jogo e diagnóstico de CPU**, usando núcleo 68000, RAM 64 KiB, TMSS e I/O. Snapshots conferem versão, integridade do conteúdo e identidade da ROM antes de aplicar as alterações. A operação é atômica em caso de validação falha; selecionar outro número de slot não gera dados inventados. **Somente um slot fica ativo por vez**, não há persistência após desligar.

A funcionalidade não suporta estado completo de Sonic, Z80, YM2612, PSG, VDP, periféricos ou scheduler; não prometer compatibilidade com jogos.

## Progresso com Sonic

Captura anterior reportava PC reset 00000206, opcode 4AB9 (TST.L abs.L), passos zero. A implementação ganhou TST.W/TST.L de endereço absoluto, condições Bcc, LEA PC relativo, MOVEM pós-incremento, MOVE.B e ANDI, além de um mapa inicial de registradores de controle e TMSS. Isso remove o primeiro bloqueio, mas **novas instruções e leituras do VDP ainda interrompem Sonic**, que segue **não jogável**. O BIOS da Sega Saturn não equivale a um Mega Drive.

A ROM comercial Sonic enviada pelo usuário é incorporada apenas em um BIN/CUE privado de teste, por substituição offline do slot reservado; permanece fora do repositório público.

## Limitações técnicas pendentes

CPU 68000 completa e suas exceções/flags/ciclos; Z80 completo; execução sincronizada de dois processadores; VDP inteiro (DMA, sprites, scroll, prioridades, status, tempos H/VBlank); emulação Yamaha YM2612 fiel; PSG compatível; áudio em tempo real por buffers SCSP; sistema de saves da plataforma; port nativo do PicoDrive; carregamento ROMs autorizadas via ISO9660 e cartuchos expandidos.

### Fontes estudadas

- https://www.copetti.org/pt-br/writings/consoles/sega-saturn/
- https://www.copetti.org/writings/consoles/mega-drive-genesis/
- https://github.com/yaul-org/libyaul
- https://github.com/yaul-org/libyaul-examples
- https://github.com/notaz/picodrive/blob/master/COPYING
- https://www.segasaturnshiro.com/2024/05/16/new-saroo-firmware-update-brings-big-changes/

O PicoDrive está fixado como submódulo opcional para análise; NÃO está integrado ao binário emulador, e seu código possui obrigações de licença e limitações para distribuição comercial.
