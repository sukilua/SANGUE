# SANGUE — by suki

Projeto novo em C++20 para Windows x64. Interface própria com banner visível, carregamento animado e ícone incorporados ao executável. Não depende de Python ou .NET.

## Estado real

Implementados: interface, preferências, diagnóstico com versão do build, busca do processo, verificação SHA256, leitor experimental de câmera/registro/ossos, overlay skeleton e controlador de mira Rage por entrada do Windows. Não há modo Normal nem skeleton de demonstração.

**Não validado dentro do Blood Strike. Não inclui bypass, driver, injeção, ocultação, obfuscação nem garantia antiban.** O alvo de espera interna é 1 ms; isso não mede nem garante a reação completa. O diagnóstico informa duração da leitura e maior ciclo observado.

O layout só é aceito para o SHA256 `ff080393961ac709090824356621358ae4f63dbe1c62ff636d528589cd097747`. A origem dos offsets é a auditoria estática existente, usada como referência técnica; o código foi escrito neste projeto. A cadeia engine/gameplay, exclusão do jogador local, equipe, vida e visibilidade ainda não foram confirmadas em execução.

Por isso, a mira fica bloqueada por padrão até permitir explicitamente **Alvos sem equipe**. Essa opção pode incluir aliados, o próprio jogador ou objetos classificados incorretamente. Não existe filtro confirmado. O número de entradas aceitas pelo Windows não prova que o jogo as aplicou.

## Abrir

Baixe o artefato SANGUE-Windows-x64 em Actions, extraia e abra SANGUE.exe. O ícone é a segunda imagem enviada. Para criar um atalho, use Enviar para → Área de trabalho no Windows. A imagem e o ícone estão incorporados, sem arquivos de arte externos.

Todos os módulos iniciam desligados. INSERT mostra/oculta o painel; END encerra. A mira é habilitada diretamente por Ativar Rage, sem chave adicional de armar; só envia movimentos com o jogo em foco e a tecla configurada pressionada. Segure Mouse 2 (ou a tecla configurada) para a mira. As preferências e o diagnóstico são armazenados em `%LOCALAPPDATA%/SANGUE`.

Skeleton: snapshots completos, IDs únicos e descarte após 25 ms. O buffer é limpo a cada desenho. Não existe previsão de movimento ou interpolação. Isso reduz caminhos de duplicação/atraso, mas não prova alinhamento perfeito em partida. Falhas ou prazos excedidos suprimem a pose.

## Compilar e testar

```text
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Testes de projeção, validade da câmera, idade das poses, deduplicação, seleção de alvo, leitura do próprio processo e SHA256. O smoke test renderiza carregamento e as três abas. Testes automatizados não validam o jogo.

## 1.1.0

Removida a dependência de F6. O painel e o relatório exibem o motivo da mira não atuar. A permissão explícita de alvos sem equipe continua necessária, porque filtros de equipe, vida e jogador local não foram confirmados.

Nomes de ossos são resolvidos uma vez por descriptor em cada amostra, sem cache entre frames. Um descriptor inválido deixa de descartar toda a entidade. Slots com nomes duplicados são suprimidos. O prazo de leitura retém apenas entidades já completas, sem reaproveitar poses de frames anteriores. Troca de skeleton ou anchor invalida a entidade. A câmera é relida no desenho; os pontos são reprojetados no referencial atual, sem misturar ossos de duas poses. Falha dessa câmera limpa o desenho e é registrada no diagnóstico.

Falhas transitórias de layout não refazem o hash do executável a cada erro. O diagnóstico preserva a última amostra em foco do mesmo PID, identifica sua origem e sua idade e distingue o estado atual. Isso não confirma que offsets, alinhamento ou sensibilidade da mira estão corretos em jogo.

Bypass e ofuscação não foram implementados.
