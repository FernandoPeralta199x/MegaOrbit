# MegaOrbit — Emulador de Mega Drive para Sega Saturn

**Projeto experimental, não um emulador Mega Drive completo.** Repositório oficial: [FernandoPeralta199x/MegaOrbit](https://github.com/FernandoPeralta199x/MegaOrbit).

Código em C99, independente de emuladores comerciais, dividido entre um núcleo portátil do Motorola 68000 e um programa de diagnóstico para Saturn usando [libyaul](https://github.com/yaul-org/libyaul).

## O que funciona nesta entrega

- Reset da CPU 68000 por vetores: A7 em ROM[0..3] e PC em ROM[4..7].
- Registradores D0–D7, A0–A7, PC, SR, contador de ciclos e estados de erro.
- `NOP`, `MOVEQ` com sinal, `BRA.s`, `BRA.w`, `BSR.s`, `BSR.w`, `RTS`, `JMP (xxx).L` e `JSR (xxx).L`.
- Barramento **parcial** de 24 bits: ROM de cartucho até 4 MB de janela (armazenamento externo), RAM principal de 64 KB e acessos big-endian.
- Erros controlados para endereço desalinhado, região não mapeada e opcode ainda não implementado. **Exceções reais da CPU ainda não são simuladas**.
- 12 testes automatizados com GCC no computador.
- Diagnóstico visual Saturn via DBGiO/VDP2, com Makefile baseado no SDK Yaul.

## Não implementado

VDP do Mega Drive, Z80, YM2612, PSG, controle, DMA, timers, interrupções, SRAM, mapper, leitura de ROM do CD, scheduler completo e a maior parte das instruções 68000. **Ainda não executa jogos comerciais.**

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

4. O nome-base pretendido do diagnóstico é `megaorbit-diag`. Teste a imagem gerada em um emulador Saturn compatível e depois em hardware real.

**Importante:** o código C99 portátil foi testado em computador; a build para SH-2/libyaul **ainda não foi validada** neste ambiente.

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
