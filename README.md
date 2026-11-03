# SANGUE — by suki / 1.4.0

Aplicativo C++20 para Windows x64, somente com mira Rage. Banner, ícone, carregamento e calibração incorporados. Sem dependência de Python ou .NET.

## Mudanças desta versão

- Removidos o ESP, a aba Skeleton, os controles visuais, a janela de overlay, o desenho de ossos e as configurações desse módulo.
- Não há pausa fixa de 8 ms ou suavização temporal de 35 ms no controlador. Cada observação nova pode gerar correção imediatamente, com os dados já disponíveis. Observações repetidas sem resposta da câmera ou movimento relevante do alvo não repetem a mesma entrada.
- Mantidos o alvo retido, a calibração X/Y, a correção angular e a zona de parada para reduzir oscilação.
- O leitor de ossos permanece apenas como fonte interna de posições da mira; não há ESP de fundo nem overlay oculto.

**Latência física de zero segundos não existe e não foi medida em partida.** A leitura, o processamento, o agendamento do Windows, a entrada e os frames do jogo somam latência. O leitor continua com espera de aproximadamente 1 ms entre ciclos para não ocupar a CPU em loop contínuo. Isso não garante tempo total de reação de 1 ms.

## Usar

Baixe SANGUE.exe ou extraia o ZIP. Todos os módulos iniciam desligados. INSERT mostra/oculta o painel e END encerra. Não é necessário F6.

No campo de treino, abra Ajustes → Calibrar sensibilidade X e Y; volte ao jogo em três segundos. Use o mesmo zoom, DPI e sensibilidade da mira. Não mova o mouse até terminar. A rotina envia e tenta desfazer pequenos movimentos de 16 counts por eixo. Perder foco ou falhar na leitura cancela a calibração. Salve as preferências depois.

Ative Rage e permita explicitamente Alvos sem equipe; segure a tecla configurada (padrão Mouse 2) com o jogo em foco. Filtros de aliados, vida, visibilidade e jogador local não foram confirmados. Alvos sem equipe podem incluir aliados ou o próprio jogador. Alterar DPI, sensibilidade ou zoom exige recalibrar. A correção é bloqueada quando o FOV difere mais de dois graus da calibração.

Preferências e diagnóstico: `%LOCALAPPDATA%/SANGUE`. O relatório mantém a última amostra em foco do mesmo PID e identifica sua origem e idade.

## Limites e validação

Leitor experimental limitado ao SHA256 `ff080393961ac709090824356621358ae4f63dbe1c62ff636d528589cd097747`. A leitura usa offsets da auditoria estática existente; a semântica da cadeia de câmera e a classificação das entidades ainda precisam de confirmação em execução.

Não validado dentro do Blood Strike. Não há bypass, ofuscação, driver, injeção ou garantia antiban. Sensibilidade não linear, aceleração, suavização do jogo e entradas recusadas podem impedir calibração ou atuação corretas. Uma entrada aceita pelo Windows não prova que o jogo a aplicou.

Testes automatizados verificam projeção, deduplicação, dados parciais de ossos, idade de amostras, calibração, inversão, estabilidade e sete resoluções (640×480 a 4K, além de retrato). As capturas do painel verificam layout e limites dos controles. Nenhum desses testes prova funcionamento em partida.

## Compilar

```text
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Correções 1.4.0

A espera por feedback da câmera na 1.3.0 podia permanecer ativa sem limite. Agora há uma única tentativa menor de recuperação após 40 ms sem mudança observável; persistindo a ausência de resposta, o painel pede diagnóstico, sem repetir movimentos às cegas. Uma entrada recusada pelo Windows libera a próxima tentativa do controlador. Isso corrige caminhos do código, mas não comprova a causa da falha observada pelo usuário em partida.

Calibração concluída é salva automaticamente; a opção escolhida para alvos sem equipe também é preservada. Rage continua desligado ao iniciar. O painel exibe o último estado da mira no mesmo processo mesmo após voltar à interface e distingue calibração ausente, permissão, leitura e retorno da câmera.

Interface com estado do controle em destaque e abas Rage, Conexão e Calibração. Carregamento com lista de etapas reais, transição visual mais curta (mínimo de 360 ms), tratamento de falha e botão Tentar novamente. Um teste Windows simula falha de recurso, captura a tela de erro e aciona a recuperação, antes de verificar as telas nas sete resoluções.

Se continuar sem atuar, exporte um novo diagnostico-SANGUE.txt depois da tentativa no campo de treino. O relatório da versão atual é necessário para distinguir ausência de entidades, bloqueio de calibração/FOV, entrada recusada e câmera sem feedback.
