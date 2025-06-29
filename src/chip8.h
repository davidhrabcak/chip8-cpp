class chip8 {
        public:
                bool get_df();
                void set_df(bool val);
                int get_display(int pos);
                int get_display_pos(int x, int y);
                int get_sp();
                void set_sp(int val);
                
                void initialize();
                void load(const char* filename);
                int emulate_cycle();

                bool key[16];
                unsigned char graphics[64 * 32]; // display

        private:
            struct registers {
                    unsigned char V[16];
                    unsigned short pc; // program counter
                };
            registers reg;
            unsigned short stack[16]; // stack for jumps
            bool df;

            unsigned char memory[4096];

            unsigned char delay_timer;
            unsigned char sound_timer;
};
