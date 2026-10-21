# MegaOrbit — Estudo dos chips e do PicoDrive

## Status de engenharia

A versão 0.3.0-dev é um **diagnóstico Sega Saturn compilável**. Não executa ROMs comerciais. Os testes de CPU, VDP e áudio são subconjuntos, não uma emulação fiel ou completa.

## PicoDrive registrado como submódulo

O código do PicoDrive está vinculado ao caminho `third_party/picodrive`, fixado em `26ecb2b6358fefba24e3d68b9eb2efba7f10d5ee`.

Para baixar sua cópia: `git clone --recurse-submodules https://github.com/FernandoPeralta199x/MegaOrbit.git`.

O binário Saturn **não compila nem vincula** o PicoDrive. Próxima fase: portar, módulo por módulo, as partes Mega Drive que forem viáveis.

### Licença

O arquivo COPYING do repositório notaz/picodrive define termos antigos da licença MAME, incluindo restrição a distribuição comercial e obrigações sobre código-fonte de derivados. Outros componentes têm autores próprios e exigem auditoria das respectivas permissões. Não presumir licença GPL ou MIT para o conjunto.

https://github.com/notaz/picodrive/blob/master/COPYING

## Componentes do Mega Drive

| Chip | Papel | Situação v0.3 |
|---|---|---|
| Motorola 68000, aproximadamente 7,67 MHz NTSC | CPU principal | Poucas instruções, reset, RAM e ROM básicas |
| Z80, aproximadamente 3,58 MHz | CPU de som | Interpretador mínimo de diagnóstico |
| VDP | Tiles, VRAM, CRAM, sprites, DMA, scroll e IRQ | VRAM/CRAM/VSRAM, comandos parciais e renderização simples de tiles; **sem saída de jogos** |
| YM2612 | Seis canais FM com quatro operadores por canal, DAC | **Aproximação de FM**, não chip fiel |
| PSG SN76489 | Três tons e um ruído | Síntese inicial com registradores |
| Memory bus | ROM, RAM, mapeamento, controles | Barramento parcial, sem BUSREQ/RESET do Z80 |

Referência: https://www.copetti.org/writings/consoles/mega-drive-genesis/

## Componentes do Saturn

- CPUs principais: dois SH-2.
- Work RAM: 2 MB, divididos em WRAM-H e WRAM-L de 1 MB cada. ROMs maiores não cabem automaticamente.
- Vídeo: VDP1 desenha sprites e VDP2 compõe camadas e planos.
- Som: SCSP/YMF292 com 32 slots, PCM/FM, acompanhado do processador auxiliar 68EC000.
- A v0.3 gera uma sequência PCM 16-bit em RAM do SCSP e configura experimentalmente o slot 0 para loop. A saída audível ainda requer teste em emulador/hardware.

Fontes:
- https://www.copetti.org/pt-br/writings/consoles/sega-saturn/
- https://www.infochunk.com/saturn/segahtml_en/hard/scsp/hon/p04_11.htm
- https://github.com/yaul-org/libyaul

## Estratégia de portabilidade do PicoDrive

1. Auditar licenças de cada componente e dependências do PicoDrive.
2. Isolar somente o Mega Drive; adiar 32X, Mega CD e chips SVP.
3. Substituir código assembly de ARM/MIPS e opções não compatíveis por implementações C compatíveis com SH-2.
4. Construir uma camada de vídeo VDP emulado separada do driver VDP1/VDP2 do Saturn.
5. Construir driver de saída SCSP com filas de amostras e sincronização por frame.
6. Integrar loader de ROMs homebrew autorizadas via ISO9660 Saturn antes de tentar APIs SD.
7. Implementar o barramento completo, BUSREQ/RESET, interrupções e eventos de vídeo.
8. Comparar traces 68000/Z80 e capturas de pixels/áudio com referências validadas.
9. Medir RAM, consumo de CPU e desempenho no Saturn real antes de ampliar compatibilidade.
10. Se distribuir derivações do PicoDrive, cumprir integralmente as obrigações de licença.

**Importante:** um núcleo Z80 ou VDP parcial não equivale ao funcionamento do chip original. O recompiler de CPUs SH-2 encontrado no PicoDrive é para emular os processadores do console 32X em máquinas modernas; não é automaticamente um port nativo para os SH-2 físicos do Saturn.

## Como testar

No PC: `make test`.

No Sega Saturn: abra o arquivo `MegaOrbit.cue` com `MegaOrbit.bin` na mesma pasta. Entre em Opções de Sistema. Nas abas Vídeo e Hardware há diagnósticos de tiles e Z80. Na aba Áudio, pressione A/C para disparar a amostra PCM no SCSP, B para parar. Durante o diagnóstico 68000, L+R+Start abre a pausa.

**Antes de declarar áudio real funcionando:** ouvir no Mednafen/Beetle Saturn e medir ausência de distorção, travamentos e conflitos com drivers de som. A checagem CI atual garante compilação e formato do disco, não funcionamento audiovisual.
