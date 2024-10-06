
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>

#include "bitboard.h"
#include "movegen.h"
#include "position.h"
#include "types.h"
#include "uci.h"

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

template<bool Root, Color SideToMove>
uint64_t PerfT(int depth)
{
    if (depth == 0)
        return 1;

    MoveList<SideToMove> moves;

    if (depth == 1 && !Root)
        return moves.size();

    uint64_t count, nodes = 0;
    
    for (Move m : moves)
    {
        do_move<SideToMove>(m);
        count = PerfT<false, !SideToMove>(depth - 1);
        undo_move<SideToMove>(m);

        nodes += count;

        if (Root)
            std::cout << move_to_uci(m) << ": " << count << std::endl;
    }

    return nodes;
}

void debug()
{
    std::ifstream in("perft_suite.txt");
    bool          failed = false;
    
    for (std::string line; std::getline(in, line);)
    {
        std::string        token;
        std::string        fen = line.substr(0, line.find(';'));
        std::istringstream is(line.substr(line.find(';')));

        Position::set(fen);

        for (uint64_t result, expected; is >> token >> expected;)
        {
            int depth = std::stoi(token.substr(2));
            std::cout << "Perft " << depth << " " << fen << std::endl;
            
            result = Position::white_to_move() ? PerfT<false, WHITE>(depth)
                                               : PerfT<false, BLACK>(depth);

            if (result != expected)
            {
                failed = true;
                std::cout << "ERROR\n" << std::endl;
                break;
            }

            if (is.eof())
                std::cout << "OK\n" << std::endl;
        }
    }

    std::cout << (failed ? "FAILED\n" : "ALL OK\n") << std::endl;
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
            int   depth;
            is >> depth;

            auto start = std::chrono::steady_clock::now();
            uint64_t result = Position::white_to_move() ? PerfT<true, WHITE>(depth)
                                                        : PerfT<true, BLACK>(depth);
            auto end   = std::chrono::steady_clock::now();
            
            std::cout << "\nnodes searched: " << result << "\nin "
                      << (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000) << " ms\n" << std::endl;
        }
        
        if (token == "position") position(is);
        if (token == "debug")    debug();
        if (token == "d")        std::cout << Position::to_string() << std::endl;
        
    } while (cmd != "quit");
}
