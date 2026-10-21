# MegaOrbit — Emulador de Mega Drive para Sega Saturn

**Projeto experimental, não um emulador Mega Drive completo.** Repositório oficial: [FernandoPeralta199x/MegaOrbit](https://github.com/FernandoPeralta199x/MegaOrbit).

Código em C99, independente de emuladores comerciais, dividido entre um núcleo portátil do Motorola 68000 e um programa de diagnóstico para Saturn usando [libyaul](https://github.com/yaul-org/libyaul).

## Teste Sega Saturn — BIN + CUE gerados

**Build v0.1.0-dev:** compilação SH-2/libyaul concluída com sucesso no [GitHub Actions](https://github.com/FernandoPeralta199x/MegaOrbit/actions/runs/39529218491). A imagem de disco foi convertida para **BIN raw MODE1/2352 + CUE**, com 266 setores, e passou nas verificações estruturais de cabeçalho Saturn e ISO9660.

[Baixar os dois arquivos da compilação](https://github.com/FernandoPeralta199x/MegaOrbit/actions/runs/39529218491) na seção *Artifacts* (faça login no GitHub, se necessário).

Abra **MegaOrbit.cue**, mantendo **MegaOrbit.bin** na mesma pasta, no Mednafen/Beetle Saturn ou em outro emulador Saturn compatível. O menu de teste em PT-BR possui seleção de diagnóstico, opções, saves demonstrativos e créditos; use D-Pad para navegar, A/C para entrar e B para voltar.

**Status de validação:** o código compilou para SH-2 e o disco passou nos verificadores; a execução visual com BIOS num emulador Saturn ou hardware real ainda precisa de teste manual. Não há execução de ROMs Mega Drive comerciais ou acesso ao cartão SD.

## MegaOrbit 0.3.0-dev — Bin/Cue com testes de chips

- [Build de diagnóstico SH-2 no GitHub Actions](https://github.com/FernandoPeralta199x/MegaOrbit/actions/runs/39532053253) — baixar o artifact MegaOrbit-Saturn-BIN-CUE.
- [Pesquisa técnica Saturn / Mega Drive / PicoDrive](docs/CHIPS_E_PICODRIVE.md).
- [PicoDrive](third_party/picodrive) como submódulo opcional e não linkado, sujeito à licença MAME antiga.
- O C99 próprio contém 68000 parcial, Z80 parcial, PSG inicial, aproximação FM, VDP simplificado e teste de PCM para SCSP.
- Navegação Saturn: D-Pad; A/C confirmar; B voltar; L+R+Start abre pausa na tela de diagnóstico.
- Abas Vídeo, Áudio, Hardware e Controles exibem testes; Áudio A/C solicita PCM looping no SCSP, B interrompe.
- Os 12 testes 68000 e a suíte de hardware passam no PC; o build SH-2 e a validação estrutural BIN/CUE passaram. **Nenhuma execução audiovisual em hardware real foi certificada nesta entrega.**

**Este pacote não executa ROMs de jogos e não é um port funcional de PicoDrive.**

## O que funciona nesta entrega

- Reset da CPU 68000 por vetores: A7 em ROM[0..3] e PC em ROM[4..7].
- Registradores D0–D7, A0–A7, PC, SR, contador de ciclos e estados de erro.
- `NOP`, `MOVEQ` com sinal, `BRA.s`, `BRA.w`, `BSR.s`, `BSR.w`, `RTS`, `JMP (xxx).L` e `JSR (xxx).L`.
- Barramento **parcial** de 24 bits: ROM de cartucho até 4 MB de janela (armazenamento externo), RAM principal de 64 KB e acessos big-endian.
- Erros controlados para endereço desalinhado, região não mapeada e opcode ainda não implementado. **Exceções reais da CPU ainda não são simuladas**.
- 12 testes automatizados com GCC no computador.
- Diagnóstico visual Saturn via DBGiO/VDP2, com Makefile baseado no SDK Yaul.

## Não implementado

VDP do Mega Drive, Z80, YM2612, PSG, controles Mega Drive emulados, DMA, timers, interrupções, SRAM, mapper, leitura de ROM do CD, scheduler completo e a maior parte das instruções 68000. **Ainda não executa jogos comerciais.**

## Testar no PC (Linux, MSYS2 ou WSL)

```sh
git clone https://github.com/FernandoPeralta199x/MegaOrbit.git
cd MegaOrbit
make test
```

Resultado esperado: `RESULTADO: 12/12 testes aprovados`.

## Compilar o diagnóstico para Sega Saturn

1. Instale a toolchain SH-2 e o SDK [libyaul](https://github.com/yaul-org/libyaul), consultando também os [exemplos oficiais](https://github.com/yaul-org/libyaul-examples).
2. Abra seu terminal MSYS2 e carregue, quando aplicável, a configuração Yaul: `source "$HOME/.yaul.env"`.
3. Entre na pasta `saturn` e compile:

   ```sh
   cd MegaOrbit/saturn
   make
   ```

4. O nome-base atual da demonstração é `megaorbit-demo`. Teste a imagem gerada em um emulador Saturn compatível e depois em hardware real.

**Importante:** a compilação cruzada SH-2/libyaul e a geração BIN/CUE foram realizadas no GitHub Actions; a emulação executável no Saturn ainda precisa de verificação.

## Diagnóstico Saturn esperado

| Etapa | PC inicial | Instrução | Resultado |
|---|---|---|---|
| 1 | 000008 | `NOP` | PC=00000A |
| 2 | 00000A | `MOVEQ #5,D0` | D0=00000005 |
| 3 | 00000C | `MOVEQ #-1,D1` | D1=FFFFFFFF |
| 4 | 00000E | `BRA.s +2` | pula NOP em 000010 |
| 5 | 000012 | `MOVEQ #7,D2` | D2=00000007 e PC=000014 |

## Estrutura

```text
MegaOrbit/
├── core/
│   ├── megacore.h
│   └── megacore.c
├── tests/
│   └── test_core.c
├── saturn/
│   ├── main.c
│   └── Makefile
├── docs/
│   └── ROADMAP.md
└── Makefile
```

## Origem e limitações

Este repositório foi inicializado com um **kit de testes independente do Motorola 68000**. As capturas anteriores mostravam um diagnóstico local em `/c/SaturnDev/MegaSaturn`, mas **esse código local antigo não foi importado**. O núcleo deste repositório é uma implementação inicial separada.

Use apenas ROMs e BIOS cuja utilização seja permitida. O repositório não inclui ROMs comerciais, BIOS ou firmware proprietário.

## Referências

- [libyaul](https://github.com/yaul-org/libyaul).
- [libyaul-examples](https://github.com/yaul-org/libyaul-examples).
- [Sega Saturn Overview Manual](https://www.infochunk.com/saturn/segahtml_en/hard/intr/hon/p02_10.htm).
