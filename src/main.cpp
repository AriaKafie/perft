
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
        for (;is >> token; fen += token + " ");

    Position::set(fen);
}

template<bool Root, Color SideToMove>
uint64_t PerfT(int depth)
{
    if (depth == 0)
        return 1;

    Move list[128], *end = generate_moves<SideToMove>(list);

    if (depth == 1 && !Root)
        return end - list;

    uint64_t count, nodes = 0;
    
    for (Move *m = list; m != end; m++)
    {
        do_move<SideToMove>(*m);
        count = PerfT<false, !SideToMove>(depth - 1);
        undo_move<SideToMove>(*m);

        nodes += count;

        if (Root)
            std::cout << move_to_uci(*m) << ": " << count << std::endl;
    }

    return nodes;
}

void debug()
{
    std::ifstream in("perft_suite.txt");
    bool failed = false;
    
    for (std::string token; std::getline(in, token);)
    {
        Position::set(token.substr(0, token.find(';')));
        
        std::istringstream is(token.substr(token.find(';')));

        for (uint64_t depth = is.str()[2] - '0', expected; is >> token >> expected; depth++)
        {
            std::cout << "Perft " << depth << " " << Position::fen() << std::endl;
            
            uint64_t result = Position::white_to_move() ? PerfT<false, WHITE>(depth)
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
    MoveGen::init();
    Position::set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    
    std::string cmd;

    do
    {
        std::getline(std::cin, cmd);
        std::istringstream is(cmd);
        
        is >> cmd;

        if (cmd == "perft")
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
        
        if (cmd == "position") position(is);
        if (cmd == "debug")    debug();
        if (cmd == "d")        std::cout << Position::to_string() << std::endl;
        
    } while (cmd != "quit");
}
