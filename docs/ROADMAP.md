# MegaOrbit — plano técnico

## Objetivo

Executar, no futuro, jogos Sega Mega Drive / Genesis em hardware Sega Saturn, com um núcleo 68000 C99 portátil otimizado para SH-2 e saída VDP1/VDP2/SCSP via libyaul. Um diagnóstico funcional da CPU **não** equivale a um emulador de jogos.

## Arquitetura pretendida

| Módulo | Hardware Mega Drive | Implementação projetada |
|---|---|---|
| m68k | Motorola 68000 (~7,67 MHz NTSC) | Interpretador em SH-2 e perfis de desempenho |
| bus | ROM, RAM e I/O | Dispatch por janelas, endianness e falhas explícitas |
| vdp | VRAM, CRAM, VSRAM, tiles/sprites | Adaptação VDP2/VDP1, fallback quando necessário |
| z80 | Zilog Z80 (~3,58 MHz NTSC) | Interpretador e arbitragem de barramento |
| fm | YM2612 | FM por software ou aproximação no SCSP |
| psg | SN76489 | Geração PSG PCM com buffer |
| scheduler | Ciclos de CPU, scanlines e IRQ | Orçamento de ciclos e sincronização |
| input | Controles 3/6 botões | Mapeamento do controle Saturn |
| storage | ROM, SRAM e mappers | Leitura autorizada, cache e formatos suportados |

## Memória Mega Drive (referência inicial)

- `000000–3FFFFF`: ROM do cartucho; extensões/bancos precisam de suporte específico.
- `A00000–A01FFF`: RAM Z80, com arbitragem.
- `A04000–A04003`: portas YM2612.
- `A10000–A1001F`: I/O de controles e versão.
- `A11100`: Z80 BUSREQ.
- `A11200`: Z80 RESET.
- `C00000–C0001F`: interface do VDP.
- `FF0000–FFFFFF`: RAM principal 64 KB.

**Hoje o código implementa apenas ROM e RAM**. Não trate valores arbitrários das demais regiões como comportamento correto.

## Etapas

### 0. Base de diagnóstico

- [x] Ler vetores de reset e inicializar PC/SP.
- [x] `NOP`, `MOVEQ`, `BRA`, `BSR`, `RTS`, `JMP` absoluto longo e `JSR` absoluto longo.
- [x] Barramento ROM/RAM básico, leitura big-endian e 12 testes no PC.
- [ ] Validar compilação SH-2 com libyaul e execução em Saturn real.

### 1. CPU confiável

- [ ] Implementar conjunto de instruções Motorola 68000 original e endereçamentos.
- [ ] Flags X/N/Z/V/C, supervisor/user, USP/SSP, exceções e interrupções.
- [ ] STOP, RESET, TRAP, instruções privilegiadas e timing por operação.
- [ ] Comparar traces com CPU 68000 de referência e aumentar regressões.

### 2. Vídeo

- [ ] Registradores VDP, VRAM/CRAM/VSRAM.
- [ ] Tiles, planos, rolagem, prioridades, sprites e paleta.
- [ ] DMA VDP, H/VBlank, scanlines e IRQs.
- [ ] Padrão gráfico de teste executado via VDP emulado e comparado com referência.

### 3. Áudio e controles

- [ ] Z80, BUSREQ/RESET, YM2612 e SN76489.
- [ ] Buffer de áudio e saída SCSP.
- [ ] Controles de três e seis botões.

### 4. ROMs e desempenho

- [ ] Carregar ROM homebrew autorizada do CD/ISO ou suporte compatível.
- [ ] SRAM, banco de ROM e variantes NTSC/PAL.
- [ ] Medir custo do interpretador no SH-2; otimização apenas baseada em medições.
- [ ] Investigar expansão de memória/cache: Saturn tem RAM limitada para ROMs grandes.
- [ ] Rodar primeiro homebrews simples e então testes de compatibilidade permitidos.

## Qualidade e honestidade dos resultados

- Um teste de opcode aprovado não demonstra execução de um jogo.
- Testar flags, PC, RAM, limites, exceções e ciclos.
- Não fabricar leituras de registradores ainda não emulados.
- Anotar a versão do compilador/libyaul e o hardware usado.
- Conferir as licenças antes de copiar códigos de projetos externos.
- Não prometer 60 FPS em todas as ROMs: as diferenças entre os dois consoles criam gargalos importantes.
