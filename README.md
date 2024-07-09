# virtual-memory-manager

Este projeto implementa um gerenciador de memória virtual baseado no exercício "Designing a Virtual Memory Manager" do livro Operating System Concepts de Silberschatz, A. et al., 10ª edição, com algumas modificações.

# Especificações do Projeto

- Memória Física: A memória física possui 128 frames.
- linguagem: Implementado em C.
- Compilação: O programa deve ser compilado utilizando make e deve gerar um arquivo executável chamado vm.
- Validação: Será utilizado o compilador gcc 13.2.0 para validação.
- Preenchimento de Frames: Os frames na memória física devem ser preenchidos do 0 ao 127. Quando a memória estiver cheia, será aplicado um algoritmo de substituição para identificar qual frame será atualizado.
- Algoritmos de Substituição:
- Implementar dois algoritmos de substituição de página: FIFO e LRU.
- Na TLB, será aplicado apenas o FIFO.

# Argumentos de Linha de Comando:

O primeiro argumento será um arquivo de endereços lógicos (similar ao address.txt). <br>
O segundo argumento será o tipo de algoritmo de substituição de página (fifo ou lru). <br>

Exemplo: ./vm address.txt lru ( utilizando o lru neste caso).

# Saída

A saída correta do FIFO está no arquivo correct.txt.
