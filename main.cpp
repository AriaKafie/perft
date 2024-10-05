
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "bitboard.h"
#include "movegen.h"
#include "position.h"
#include "types.h"
#include "uci.h"

template<Color SideToMove>
uint64_t PerfT(int depth)
{
    if (depth == 0)
        return 1;

    MoveList<SideToMove> moves;

    if (depth == 1)
        return moves.size();

    uint64_t nodes = 0;
    
    for (Move m : moves)
    {
        do_move<SideToMove>(m);
        nodes += PerfT<!SideToMove>(depth -1);
        undo_move<SideToMove>(m);
    }

    return nodes;
}

template<Color SideToMove>
void debug(std::istringstream& is)
{
    int   depth;
    is >> depth;
    
    uint64_t elapsed = 0, total_nodes = 0;

    MoveList<SideToMove> moves;

    for (Move m : moves)
    {
        uint64_t nodes = 0;

        do_move<SideToMove>(m);
        nodes = PerfT<!SideToMove>(depth - 1);
        undo_move<SideToMove>(m);

        total_nodes += nodes;

        std::cout << move_to_uci(m) << ": " << nodes << std::endl;
    }

    std::cout << "\nnodes searched: " << total_nodes << "\n" << std::endl;
}

void position(std::istringstream& is)
{
    std::string token, fen = "";

    is >> token;

    if (token == "startpos")
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    else
        while (is >> token)
            fen += token + " ";

    Position::set(fen);
}

void test()
{
    std::string line, token;
    std::ifstream in("perft_suite.txt");
    uint64_t expected;

    while (std::getline(in, line))
    {
        std::istringstream is(line);
        std::string fen = "";

        while (is >> token && token != "1")
            fen += token + " ";

        Position::set(fen);
        
        while (is >> token >> expected)
        {
            int depth = std::stoi(token.substr(2));

            std::cout << "PerfT " << depth << " " << fen << std::endl;

            uint64_t nodes = Position::white_to_move() ? PerfT<WHITE>(depth)
                                                       : PerfT<BLACK>(depth);
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

        if (token == "perft")
        {
            if (Position::white_to_move()) debug<WHITE>(is);
            else                           debug<BLACK>(is);
        }
        
        if (token == "position") position(is);
        if (token == "debug")    test();
        if (token == "d")        std::cout << Position::to_string() << std::endl;
        
    } while (cmd != "quit");
}

