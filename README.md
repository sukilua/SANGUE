# SANGUE — by suki / 1.5.0

Projeto C++20 para Windows x64, somente com mira Rage. Sem ESP, modo Normal ou dependência de F6. Banner, ícone, carregamento, diagnóstico e calibração incorporados.

## Acompanhamento contínuo

Com Rage ligado, o jogo em foco e a tecla configurada pressionada, o controlador avalia cada nova amostra e continua corrigindo o mesmo alvo. A trava de espera por resposta da câmera e o limite de duas tentativas da versão anterior foram removidos.

Quando o alvo chega ao centro, o mouse deixa de receber correções desnecessárias, mas o acompanhamento permanece ativo. Se o alvo se mover, mudar de direção ou reaparecer no FOV, as correções retomam automaticamente. Soltar a tecla, desligar Rage, perder foco ou ter dados expirados pausa o controle.

A correção após a aquisição é proporcional ao tempo entre amostras e mantém zona de parada, resíduos fracionários e limite de deslocamento. Isso reduz envio duplicado do mesmo snapshot e movimentos excessivos, sem parar permanentemente após a primeira correção. Sensibilidade, resolução e FOV continuam considerados. Isso não prova ausência de oscilação em partida.

## Usar

1. Abra SANGUE.exe. O módulo Rage inicia desligado.
2. No campo de treino, abra Calibração e clique Calibrar sensibilidade X e Y. Volte ao jogo em três segundos, com o mesmo zoom/DPI usado pela mira. Não mova o mouse até concluir. A rotina envia e tenta desfazer movimentos de 16 counts por eixo. A calibração é salva automaticamente.
3. Ative Rage contínuo e permita explicitamente Alvos sem equipe. Segure Mouse 2 (ou a tecla configurada) para acompanhar o alvo.
4. INSERT mostra/oculta o painel; END encerra.

Alvos sem equipe podem incluir aliados ou o próprio jogador: equipe, vida, visibilidade e jogador local não possuem filtro confirmado. Alterar DPI, sensibilidade ou zoom exige recalibrar. Diferença de FOV maior que dois graus em relação à calibração bloqueia a mira e informa o motivo.

Preferências e diagnóstico ficam em `%LOCALAPPDATA%/SANGUE`. O painel destaca o estado do controle e preserva a última amostra em foco do mesmo PID. Se não houver atuação, exporte um diagnóstico desta versão após tentar no treino.

## Validação e limites

Testes sintéticos incluem acompanhamento prolongado, retorno após alinhamento ao centro, inversão de direção de alvo móvel, perda e recuperação de alvo, calibração, inversão de sensibilidade e sete resoluções. Testes Windows verificam leitura do próprio processo, casos de dados de ossos parciais, inicialização, recuperação de falha e telas do painel.

**Ainda não validado dentro do Blood Strike.** Nenhum teste automatizado prova atuação correta no jogo. Entradas aceitas pelo Windows podem ser ignoradas pelo jogo. A cadeia de câmera e a classificação das entidades ainda precisam de confirmação em execução.

O leitor só aceita o SHA256 `ff080393961ac709090824356621358ae4f63dbe1c62ff636d528589cd097747`. Os ossos são lidos apenas como posições internas da mira; não há desenho nem janela de overlay.

Sem bypass, ofuscação, driver, injeção ou garantia antiban. Latência real de zero segundos não é possível. O ciclo mantém aproximadamente 1 ms de espera entre leituras para não ocupar continuamente a CPU; esse intervalo não mede a latência completa da reação.

## Compilar

```text
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```
