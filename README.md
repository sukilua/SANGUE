# SANGUE — by suki / 1.6.0

Projeto C++20 para Windows x64, com mira Rage, banner, ícone, carregamento e diagnóstico. Sem ESP, modo Normal ou dependência de F6.

## Controle sem calibração

A etapa de calibração e o bloqueio por diferença de zoom foram removidos do painel, das preferências carregadas e do controlador. Configurações antigas de calibração são ignoradas. O programa não envia mais movimentos de medição de sensibilidade.

O controlador calcula o desvio entre o osso selecionado e o centro da tela, normalizado para uma altura de 1080 pixels. Cada amostra nova permite uma correção proporcional ao tempo; a mesma amostra não é enviada duas vezes. A câmera atual continua sendo usada para projetar os ossos durante mudanças de zoom.

O alvo é mantido enquanto permanecer válido dentro do FOV. Chegar ao centro pausa os movimentos, mantendo o acompanhamento; sair do centro retoma as correções. Valores fracionários são acumulados para evitar uma parada permanente causada pelo arredondamento. Isso não comprova ausência de oscilação em partida.

## Usar

1. Abra SANGUE.exe. Rage inicia desligado.
2. No treino, ative Rage contínuo e permita explicitamente Alvos sem equipe.
3. Segure Mouse 2, Mouse 4 ou Shift, conforme a tecla escolhida no painel.
4. Em Controle, ajuste a força de 25% a 200%. Se ultrapassar o alvo, reduza; se corrigir devagar, aumente. Há inversão independente dos eixos X e Y.
5. INSERT mostra ou oculta o painel; END encerra.

A correção é normalizada por resolução, mas a resposta física continua dependendo da sensibilidade, DPI e tratamento de entrada do jogo. Remover a medição não torna todas as sensibilidades equivalentes. A força padrão é 100%; ajuste no treino quando necessário.

Preferências são salvas automaticamente ao alterar os controles. Rage permanece desligado ao abrir novamente. Preferências e diagnóstico ficam em `%LOCALAPPDATA%/SANGUE`. O relatório inclui força, inversão dos eixos, FOV atual e o estado da última amostra em foco do mesmo PID.

## Validação e limites

Testes sintéticos verificam acompanhamento prolongado, alvo móvel, perda e recuperação do alvo, correções fracionárias, inversão dos eixos, sete resoluções e mudanças de zoom sem bloqueio por calibração. Testes Windows verificam leitura do próprio processo, dados de ossos parciais, inicialização, recuperação de falha e telas do painel.

**Ainda não validado dentro do Blood Strike.** Entradas aceitas pelo Windows podem ser ignoradas pelo jogo. A cadeia de câmera e a classificação das entidades precisam de confirmação em execução. Equipe, vida, visibilidade e jogador local não possuem filtro confirmado; Alvos sem equipe pode incluir aliados ou o próprio jogador.

O leitor só aceita o SHA256 `ff080393961ac709090824356621358ae4f63dbe1c62ff636d528589cd097747`. Os ossos são lidos apenas como posições internas da mira; não há desenho nem janela de overlay. O foco, a tecla e a validade da amostra são conferidos novamente imediatamente antes de enviar movimento.

Sem bypass, ofuscação, driver, injeção ou garantia antiban. O ciclo mantém aproximadamente 1 ms de espera entre leituras; esse intervalo não mede a latência completa da reação.

## Compilar

```text
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
