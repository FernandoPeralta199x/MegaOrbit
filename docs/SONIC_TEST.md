# MegaOrbit — Teste de ROM Sonic / GRIFFONBR (v0.4.0-dev)

## Resumo

A interface principal do Saturn ganhou **GRIFFONBR** e o texto **Version 1.0.0 [MENU]** (identidade do menu, não da compatibilidade). A lista de ROMs passou a conter **SONIC THE HEDGEHOG.MD** e **DIAGNOSTICO 68000**.

## ROM analisada (não publicada no GitHub)

Arquivo fornecido pelo usuário: **Sonic The Hedgehog (USA, Europe)(1).md**, tamanho **524288 bytes**, cabeçalho **SEGA MEGA DRIVE**, região **JUE**, vetor inicial SP=0x00FFFE00, PC=0x00000206, primeiro opcode **0x4AB9**.

O primeiro opcode é `TST.L (xxx).L` e o núcleo 68000 ainda não o implementa. Portanto o menu **consegue verificar que a ROM foi incluída e iniciar o teste**, mas **o jogo Sonic NÃO roda**. Os módulos Z80, VDP, YM2612 e PSG permanecem incompletos. Nenhum vídeo ou gameplay de Sonic foi produzido.

## Como montar sem distribuir a ROM

No GitHub a compilação gera somente um **espaço reservado inerte de 512 KB**, marcado por `MEGAORBIT-SONIC-SLOT-V1`, dentro do executável SH-2. O comando do CI cria `saturn/assets/sonic_slot.bin` temporariamente com `scripts/make_sonic_slot.py`. O arquivo original do jogo não é enviado ao GitHub.

O artefato do workflow gera o disco BIN/CUE. Para testes particulares é possível inserir, **fora do GitHub**, uma ROM de propriedade do usuário nesse slot, gerando de novo as checksums EDC/ECC de cada setor alterado.

## Resultado verificável

- [x] 12 testes 68000 e suíte Z80/PSG/FM/VDP no PC.
- [x] Compilação SH-2 com libyaul.
- [x] Estrutura BIN/CUE Saturn ISO9660 verificada.
- [x] Slot de ROM embutida preparado.
- [ ] Execução gráfica de Sonic (incompatível no atual estágio).
- [ ] Teste de boot e input desta versão em emulador Saturn real.
- [ ] Execução de opcodes e periféricos suficientes para inicializar Sonic.

## Layout do menu

```text
           MEGA ORBIT
 Sega Genesis / Mega Drive Emulator
      Version 1.0.0 [MENU]
            GRIFFONBR

 > SELECIONAR JOGO
   OPCOES DE SISTEMA
   GERENCIADOR DE SAVES
   CREDITOS

 [A/C] Confirmar  [B] Voltar  [D-Pad] Navegar
```

A renderização atual no BIN/CUE ainda utiliza texto VDP2 de diagnóstico, não o fundo ilustrado neon solicitado anteriormente. Integrar o bitmap ao pipeline VDP2 é uma etapa separada.

## Próximas metas

1. Adicionar barramento I/O do Mega Drive, incluindo registradores A100xx e teste TMSS.
2. Implementar o `TST.L` e as instruções de boot subsequentes.
3. Portar, após revisão de licença, o interpretador 68000 e o VDP do PicoDrive.
4. Implementar boot de homebrew 68000 mínimo com framebuffer e controles.
5. Medir tempo por frame e memória em Sega Saturn.
