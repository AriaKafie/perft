
#include <chrono>
#include <iostream>
#include <sstream>

#include "bitboard.h"
#include <fstream>
#include "movegen.h"
#include "position.h"
#include "types.h"
#include "uci.h"

static uint64_t nodes;

template<Color SideToMove>
void search(int depth)
{
    if (depth == 0)
    {
        nodes++;
        return;
    }

    MoveList<SideToMove> moves;

    if (depth == 1)
    {
        nodes += moves.size();
        return;
    }

    for (Move m : moves)
    {
        do_move<SideToMove>(m);
        search<!SideToMove>(depth -1);
        undo_move<SideToMove>(m);
    }
}

template<Color SideToMove>
void PerfT(int depth)
{
    uint64_t elapsed = 0, total_nodes = 0;

    MoveList<SideToMove> moves;

    for (Move m : moves)
    {
        nodes = 0;

        auto start = std::chrono::steady_clock::now();

        do_move<SideToMove>(m);
        search<!SideToMove>(depth - 1);
        undo_move<SideToMove>(m);

        auto end = std::chrono::steady_clock::now();

        elapsed += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        total_nodes += nodes;

        std::cout << move_to_uci(m) << ": " << nodes << std::endl;
    }

    std::cout << "\nnodes searched: " << total_nodes << "\nin " << (elapsed / 1000) << " ms\n" << std::endl;
}

void perft(std::istringstream& is)
{
    int depth;
    is >> depth;
    
    if (Position::white_to_move()) PerfT<WHITE>(depth);
    else                           PerfT<BLACK>(depth);
}

void position(std::istringstream& is)
{
    std::string fen = "", token;

    is >> token;

    while (is >> token)
        fen += token + " ";

    Position::set(fen);
}

void debug()
{
    std::string line, token;
    std::ifstream in("perft_suite.txt");

    while (std::getline(in, line))
    {
        std::istringstream is(line);
        std::string fen = "";

        while (is >> token && token != "1")
            fen += token + " ";

        Position::set(fen);
        
        while (is >> token)
        {
            int depth = std::stoi(token.substr(2));
            uint64_t expected;
            is >> expected;

            std::cout << "PerfT " << depth << " " << fen << std::endl;

            nodes = 0;
            if (Position::white_to_move()) search<WHITE>(depth);
            else                           search<BLACK>(depth);

            if (nodes != expected)
                std::cout << "ERROR" << std::endl;

            if (is.eof() && nodes == expected)
                std::cout << "OK\n" << std::endl;
        }
    }

    std::cout << "DONE\n" << std::endl;
}

int main()
{
    Bitboards::init();
    Position::set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::string cmd, token;

    do
    {
        std::getline(std::cin, cmd);

        std::istringstream is(cmd);
        is >> token;

        if (token == "perft")    perft(is);
        if (token == "position") position(is);
        if (token == "debug")    debug();
        if (token == "d")        std::cout << Position::to_string() << std::endl;
        
    } while (cmd != "quit");
}
