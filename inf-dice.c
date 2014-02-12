#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#define B_MAX  5
#define SAVES_MAX  3
#define SUCCESS_MAX (B_MAX * SAVES_MAX)
#define STAT_MAX 20
#define ROLL_MAX 20
#define DAM_MAX 15

// Other assumptions require that NUM_THREADS equals ROLL_MAX
#define NUM_THREADS ROLL_MAX

enum ammo_t{
    AMMO_NORMAL,
    AMMO_DA,
    AMMO_EXP,
    AMMO_FIRE,
    AMMO_NONE,
};

static const char *ammo_labels[] = {
    "Normal",
    "DA",
    "EXP",
    "Fire",
    "None",
};

/*
 * This is an implementation of Infinity dice math that enumerates every
 * possible combination given the BS and B of both models and tabulates
 * the outcomes.
 *
 * Created by Jonathan Polley.
 */

/*
 * Structure for a single die result.
 */
struct result{
    int value;          // Number that was rolled
    int is_hit;         // If the die is a hit (true on a crit)
    int is_crit;        // If the die is a crit
};

/*
 * Data structure for each player.
 *
 * Includes both player attributes and their hit/success tables.
 */
struct player{
    int stat;                   // target number for rolls
    int n;                      // number of dice
    int dam;                    // damage value
    enum ammo_t ammo;           // ammo type

    struct result d[B_MAX];     // current set of dice being evaluated
    int best;                   // offset into d - their best die

    // count of hit types
    // first index is number of regular hits
    // second index is number of crits
    // value is number of times this happened
    int64_t hit[B_MAX + 1][B_MAX + 1];

    // Number of times N successes was inflicted
    double success[SUCCESS_MAX + 1];
};

/*
 * Master data structure.
 */
struct dice{
    int thread_num;
    struct player p1, p2;

    int64_t num_rolls;
};




/*
 * print_player_hits()
 *
 * Helper for print_tables().  Prints likelyhood that player scored a
 * certain number of hits/crits.
 */
static int64_t print_player_hits(struct player *p, int p_num, int64_t num_rolls){
    int hits, crits;
    int64_t n_rolls = 0;

    for(hits = 0; hits <= B_MAX; hits++){
        for(crits = 0; crits <= B_MAX; crits++){
            if((hits > 0 || crits > 0) && p->hit[hits][crits] > 0){
                printf("P%d Hits: %d Crits %d: %6.2f%% (%lld)\n", p_num, hits, crits, 100.0 * p->hit[hits][crits] / num_rolls, p->hit[hits][crits]);
                n_rolls += p->hit[hits][crits];
            }
        }
    }
    printf("\n");

    return n_rolls;
}

/*
 * print_player_successes()
 *
 * Helper for print_tables().  Prints likelyhood that player scored a
 * certain number of successes.
 */
double print_player_successes(struct player *p, int p_num, int64_t num_rolls){
    double cumul_prob;
    int success;
    double n_success = 0;

    cumul_prob = 0.0;
    for(success = SUCCESS_MAX; success > 0; success--){
        if(p->success[success] > 0){
            double prob = 100.0 * p->success[success] / num_rolls;
            cumul_prob += prob;
            if(prob >= 0.005){
                printf("P%d Scores %2d Success(es): %6.2f%%     %2d+ Successes: %6.2f%%\n", p_num, success, prob, success, cumul_prob);
            }

            n_success += p->success[success];
        }
    }
    printf("\n");

    return n_success;
}

/*
 * print_tables()
 *
 * Prints generated data in an orderly format.
 *
 * Prints both raw hit data and success statistics.
 */
static void print_tables(struct dice *d){
    int64_t n_rolls = 0, n;
    double n_success = 0;
    double n2;

    printf("Total Hits: %lld\n", d->num_rolls);
    printf("\n");

    n_rolls += print_player_hits(&d->p1, 1, d->num_rolls);

    n = d->p1.hit[0][0] + d->p2.hit[0][0];
    n_rolls += n;
    printf("No Hits: %6.2f%% %lld\n", 100.0 * n / d->num_rolls, n);
    printf("\n");

    n_rolls += print_player_hits(&d->p2, 2, d->num_rolls);
    assert(n_rolls == d->num_rolls);

    printf("\n");
    printf("======================================================\n");
    printf("\n");

    n_success += print_player_successes(&d->p1, 1, d->num_rolls);

    n2 = d->p1.success[0] + d->p2.success[0];
    n_success += n2;
    printf("No Successes: %6.2f%%\n", 100.0 * n2 / d->num_rolls);
    printf("\n");

    n_success += print_player_successes(&d->p2, 2, d->num_rolls);
    assert(round(n_success) == d->num_rolls);
}

/*
 * factorial()
 *
 * Standard numerical function. Precalculated for efficiency.
 */
static int64_t factorial(int n){
    switch(n){
        case 0:
        case 1:
            return 1;
            break;
        case 2:
            return 2;
            break;
        case 3:
            return 6;
            break;
        case 4:
            return 24;
            break;
        case 5:
            return 120;
            break;
        default:
            return n * factorial(n - 1);
            break;
    }
}

/*
 * choose()
 *
 * Standard probability/statistics function.
 */
static int64_t choose(int n, int k){
    return factorial(n) / (factorial(k) * factorial(n - k));
}

/*
 * hit_prob()
 *
 * Uses binomial theorem to calculate the likelyhood that a certain number
 * of hits were successful.
 */
static double hit_prob(int successes, int trials, double probability){
    return choose(trials, successes) * pow(probability, successes) * pow(1 - probability, trials - successes);
}

/*
 * fire_damage()
 *
 * Helper for calc_player_successes(). Recursively calculates how many successes
 * Fire ammo could have inflicted.
 */
static void fire_damage(struct player *p, int hits, int total_hits, double prob, int depth){
    int success;

    if(depth == 0){
        // record damage at bottom of stack
        assert(total_hits <= SUCCESS_MAX);
        p->success[total_hits] += prob;
        return;
    }

    for(success = 0; success <= hits; success++){
        double new_prob = hit_prob(success, hits, ((double)p->dam) / ROLL_MAX);
        int new_depth = depth - 1;

        if(success == 0){
            // record data if no additional hits were scored.
            new_depth = 0;
        }

        fire_damage(p, success, total_hits + success, prob * new_prob, new_depth);
    }
}

/*
 * calc_player_successes()
 *
 * For a given player, traverses their hit/crit table and determines how
 * likely they are to have inflicted successes on their opponent.
 */
static void calc_player_successes(struct player *p){
    int hits, crits, success;

    for(hits = 0; hits <= B_MAX; hits++){
        for(crits = 0; crits <= B_MAX; crits++){
            if(p->hit[hits][crits] > 0){
                // We scored this many hits and crits
                // now we need to determine how likely it was we caused however many successes
                // Gotta binomialize!

                // crits always hit, so they are an offset into the success array
                // then we count up to the max number of hits.
                int saves;
                if(p->ammo == AMMO_FIRE){
                    // Fire ammo
                    // If you fail the save, you must roll again, ad infinitum.
                    fire_damage(p, hits + crits, crits, p->hit[hits][crits], SAVES_MAX - 1);
                }else if(p->ammo == AMMO_NONE){
                    // Non-lethal skill (Dodge, Smoke)
                    // There is no saving throw. Number of successes still
                    // matters for smoke.
                    p->success[crits + hits] += p->hit[hits][crits];
                }else{
                    switch(p->ammo){
                        case AMMO_DA:
                            // DA - two saves per hit, plus the second die for crits
                            saves = 2 * hits + crits;
                            break;
                        case AMMO_EXP:
                            // EXP - three saves per hit, plus the extra two for crits
                            saves = 3 * hits + 2 * crits;
                            break;
                        case AMMO_NORMAL:
                            // Normal - one save per regular hit
                            saves = hits;
                            break;
                        default:
                            printf("ERROR: Unknown ammo type: %d\n", p->ammo);
                            exit(1);
                            break;
                    }

                    for(success = 0; success <= saves; success++){
                        assert(crits + success <= SUCCESS_MAX);
                        p->success[crits + success] += hit_prob(success, saves, ((double)p->dam) / ROLL_MAX) * p->hit[hits][crits];
                    }
                }
            }
        }
    }
}

/*
 * calc_successes()
 *
 * Causes the success tables to be calculated for each player.
 */
static void calc_successes(struct dice *d){
    calc_player_successes(&d->p1);
    calc_player_successes(&d->p2);
}

/*
 * count_player_results()
 *
 * Compares each die for a given player to the best roll for the other
 * player. Then counts how many uncanceled hits/crits this player scored.
 */
static void count_player_results(struct player *us, struct player *them, int *hits, int *crits){
    int i;
    *hits = 0;
    *crits = 0;

    // Find highest successful roll of other player
    // Use the fact that the array is sorted
    them->best = 0;
    for(i = them->n - 1; i >= 0; i--){
        if(them->d[i].is_hit){
            them->best = i;
            break;
        }
    }

    for(i = us->n - 1; i >= 0; i--){
        if(us->d[i].is_hit){
            if(us->d[i].is_crit){
                // crit, see if it was canceled
                if(!(them->stat >= us->stat && them->d[them->best].is_crit)){
                    (*crits)++;
                }else{
                    // All lower dice will also be canceled
                    break;
                }
            }else{
                // it was a regular hit, see if it was canceled
                if(!them->d[them->best].is_hit || (!them->d[them->best].is_crit &&
                        (them->d[them->best].value < us->d[i].value ||
                        (them->d[them->best].value == us->d[i].value && them->stat < us->stat)))){
                    (*hits)++;
                }else{
                    // All lower dice will also be canceled
                    break;
                }
            }
        }
    }
}

/*
 * repeat_factor()
 *
 * Helper for count_roll_results()
 *
 * Counts the lengths of sequences in the die rolls in order to find the
 * factorial denominator for the data multiplier. This is easy to do since
 * the roller outputs the numbers in sorted order.
 */
static int repeat_factor(struct player *p){
    int seq_len = 1, seq_num;
    int i;
    int fact = 1;

    seq_num = p->d[0].value;
    for(i = 1; i < p->n; i++){
        if(p->d[i].value != seq_num){
            if(seq_len > 1){
                fact *= factorial(seq_len);
            }
            seq_num = p->d[i].value;
            seq_len = 1;
        }else{
            seq_len++;
        }
    }
    if(seq_len > 1){
        fact *= factorial(seq_len);
    }

    return fact;
}

/*
 * miss_factor()
 *
 * Helper for count_roll_results()
 *
 * Counts how many die rolls we didn't bother rolling because we know
 * they were going to miss.
 */
static int miss_factor(struct player *p, int start){
    int i;
    int fact = 1;

    for(i = start; i < p->n; i++){
        if(p->d[i].is_hit){
            continue;
        }
        fact *= ROLL_MAX - p->d[i].value + 1;
    }

    return fact;
}

/*
 * print_roll
 *
 * Test function that prints roll values and their multiplier.
 */
void print_roll(struct dice *d, int64_t multiplier){
    int i;
    for(i = 0; i < d->p1.n; i++){
        printf("%02d ", d->p1.d[i].value);
    }
    printf("| ");
    for(i = 0; i < d->p2.n; i++){
        printf("%02d ", d->p2.d[i].value);
    }
    printf("x %lld\n", multiplier);
}

/*
 * count_roll_results()
 *
 * For a given configuration of dice, calculates who won the FtF roll,
 * and how many hits/crits they scored.  Adds these to a running tally.
 *
 * Uses factorials to calculate a multiplicative factor to un-stack the
 * duplicate die rolls that the matrix symmetry optimization cut out.
 */
static void count_roll_results(struct dice *d){
    int hits1, crits1;
    int hits2, crits2;
    int fact1, fact2;
    int64_t multiplier;

    // Hits are counted as 'multiplier' since we are using matrix symmetries
    fact1 = repeat_factor(&d->p1);
    fact2 = repeat_factor(&d->p2);
    multiplier = factorial(d->p1.n) / fact1 * factorial(d->p2.n) / fact2;

    // more multipliers for rolling up all misses
    multiplier *= miss_factor(&d->p1, 0);
    multiplier *= miss_factor(&d->p2, 0);

    //print_roll(d, multiplier);

    count_player_results(&d->p1, &d->p2, &hits1, &crits1);
    count_player_results(&d->p2, &d->p1, &hits2, &crits2);

    assert((crits1 + hits1 == 0) || (crits2 + hits2 == 0));

    // Need to ensure we only count totally whiffed rolls once
    if(crits1 + hits1){
        d->p1.hit[hits1][crits1] += multiplier;
    }else{
        d->p2.hit[hits2][crits2] += multiplier;
    }

    d->num_rolls += multiplier;
}

/*
 * annotate_roll()
 *
 * This is a helper for roll_dice() that marks whether a given die is a
 * hit or a crit.
 *
 * XXX This should be extended to support stats over 20 with extended
 * crititcal ranges.
 */
static void annotate_roll(struct player *p, int n){
    if(p->d[n].value == p->stat){
        p->d[n].is_crit = 1;
    }else{
        p->d[n].is_crit = 0;
    }

    if(p->d[n].value <= p->stat){
        p->d[n].is_hit = 1;
    }else{
        p->d[n].is_hit = 0;
    }
}

/*
 * roll_dice()
 *
 * Recursive die roller.  Generates all possible permutations of dice,
 * calls into count_roll_results() as each row is completed.
 *
 * Uses matrix symmetries to cut down on the number of identical
 * evaluations.
 */
static void roll_dice(int b1, int b2, int start1, int start2, struct dice *d, int thread_num){
    int i, n;
    int step;
 
    // step is used for outermost loop to divide up data between threads
    // Each thread does a single digit of first die roll
    if(thread_num >= 0){
        step = ROLL_MAX;
        start1 = thread_num + 1;
    }else{
        step = 1;
    }

    if(b1 > 0){
        // roll next die for P1
        for(i = start1; i <= ROLL_MAX; i += step){
            n = d->p1.n - b1;

            d->p1.d[n].value = i;

            annotate_roll(&d->p1, n);

            // If this die is a miss, we know all higher rolls are misses, too.
            // Send in a multiplier and exit this loop early.
            // Don't do it on the first index, since that is our thread slicer
            // Don't do it on the start value, as that has a different multiplier on the back-end
            roll_dice(b1 - 1, b2, i, 1, d, -1);

            if(!d->p1.d[n].is_hit){
                break;
            }
        }
    }else if(b2 > 0){
        // roll next die for P2
        for(i = start2; i <= ROLL_MAX; i++){
            n = d->p2.n - b2;

            d->p2.d[n].value = i;

            annotate_roll(&d->p2, n);

            roll_dice(0, b2 - 1, 21, i, d, -1);

            if(!d->p2.d[n].is_hit){
                break;
            }
        }
    }else{
        // all dice are rolled; count results
        count_roll_results(d);
    }
}


/*
 * rolling_thread()
 */
void *rolling_thread(void *data){
    struct dice *d = data;

    if(d->thread_num <= d->p1.stat){
        roll_dice(d->p1.n, d->p2.n, 1, 1, d, d->thread_num);
    }

    return NULL;
}


/*
 * tabulate()
 *
 * This function generates and then prints two tables.
 *
 * First is the total number of hits/crits possible for each player.
 * This is calculated using roll_dice().
 *
 * Second is the number of successes that each of these hit outcomes could
 * cause. These are calculated by calc_successes().
 *
 * Finally, both datasets are printed using print_tables().
 */
static void tabulate(struct player *p1, struct player *p2){
    struct dice d[NUM_THREADS];
    pthread_t threads[NUM_THREADS];
    int t, h, c;
    int rval;

    for(t = 0; t < NUM_THREADS; t++){
        memset(&d[t], 0, sizeof(d[t]));

        d[t].thread_num = t;
        d[t].p1.n = p1->n;
        d[t].p2.n = p2->n;
        d[t].p1.stat = p1->stat;
        d[t].p2.stat = p2->stat;
        d[t].p1.dam = p1->dam;
        d[t].p2.dam = p2->dam;
        d[t].p1.ammo = p1->ammo;
        d[t].p2.ammo = p2->ammo;

        rval = (pthread_create(&threads[t], NULL, rolling_thread, &d[t]));
        //rolling_thread(&d[t]);
        if(rval){
            printf("ERROR: failed to create thread %d of %d\n", t, NUM_THREADS);
            exit(1);
        }
    }

    // Wait for all threads and sum the results
    pthread_join(threads[0], NULL);
    //printf("thread %d num_rolls %lld\n", 0, d[0].num_rolls);
    for(t = 1; t < NUM_THREADS; t++){
        pthread_join(threads[t], NULL);
        //printf("thread %d num_rolls %lld\n", t, d[t].num_rolls);
        d[0].num_rolls += d[t].num_rolls;

        // copy hit and crit data
        for(h = 0; h <= B_MAX; h++){
            for(c = 0; c <= B_MAX; c++){
                d[0].p1.hit[h][c] += d[t].p1.hit[h][c];
                d[0].p2.hit[h][c] += d[t].p2.hit[h][c];
            }
        }
    }
    //printf("total rolls %lld should be %.0f\n", d[0].num_rolls, pow(ROLL_MAX, d[0].p1.n + d[0].p2.n));
    assert(d[0].num_rolls == pow(ROLL_MAX, d[0].p1.n + d[0].p2.n));
    
    calc_successes(d);

    print_tables(d);
}   

static void print_player(const struct player *p, int p_num){
    printf("P%d BS %2d B %d DAM %2d AMMO %s\n", p_num, p->stat, p->n, p->dam, ammo_labels[p->ammo]);
}

int main(int argc, char *argv[]){
    struct player p1, p2;
    char ammo1, ammo2;
    int i;

    if(argc != 9){
        printf("Usage: %s <BS 1> <B 1> <DAM 1> <AMMO 1> <BS 2> <B 2> <DAM 2> <AMMO 2>\n", argv[0]);
        return 1;
    }

    i = 1;
    p1.stat = strtol(argv[i++], NULL, 10);
    p1.n = strtol(argv[i++], NULL, 10);
    p1.dam = strtol(argv[i++], NULL, 10);
    ammo1 = argv[i++][0];
    p2.stat = strtol(argv[i++], NULL, 10);
    p2.n = strtol(argv[i++], NULL, 10);
    p2.dam = strtol(argv[i++], NULL, 10);
    ammo2 = argv[i++][0];

    if(p1.n < 1 || p1.n > B_MAX){
        printf("ERROR: B 1 must be in the range of 1 to %d\n", B_MAX);
        return 1;
    }

    if(p2.n < 1 || p2.n > B_MAX){
        printf("ERROR: B 2 must be in the range of 1 to %d\n", B_MAX);
        return 1;
    }

    if(p1.stat < 0 || p1.stat > STAT_MAX){
        printf("ERROR: BS 1 must be in the range of 0 to %d\n", STAT_MAX);
        return 1;
    }

    if(p2.stat < 0 || p2.stat > STAT_MAX){
        printf("ERROR: BS 2 must be in the range of 0 to %d\n", STAT_MAX);
        return 1;
    }

    if(p1.dam < 0 || p1.dam > DAM_MAX){
        printf("ERROR: DAM 1 must be in the range of 0 to %d\n", DAM_MAX);
        return 1;
    }

    if(p2.dam < 0 || p2.dam > DAM_MAX){
        printf("ERROR: DAM 2 must be in the range of 0 to %d\n", DAM_MAX);
        return 1;
    }

    switch(ammo1){
        case 'N':
            p1.ammo = AMMO_NORMAL;
            break;
        case 'D':
            p1.ammo = AMMO_DA;
            break;
        case 'E':
            p1.ammo = AMMO_EXP;
            break;
        case 'F':
            p1.ammo = AMMO_FIRE;
            break;
        case '-':
            p1.ammo = AMMO_NONE;
            break;
        default:
            printf("ERROR: AMMO 1 type %c unknown.  Must be one of N, D, E, F, -\n", ammo1);
            exit(1);
            break;
    }

    switch(ammo2){
        case 'N':
            p2.ammo = AMMO_NORMAL;
            break;
        case 'D':
            p2.ammo = AMMO_DA;
            break;
        case 'E':
            p2.ammo = AMMO_EXP;
            break;
        case 'F':
            p2.ammo = AMMO_FIRE;
            break;
        case '-':
            p2.ammo = AMMO_NONE;
            break;
        default:
            printf("ERROR: AMMO 2 type %c unknown.  Must be one of N, D, E, F, -\n", ammo2);
            exit(1);
            break;
    }

    print_player(&p1, 1);
    print_player(&p2, 2);
    printf("\n");

    tabulate(&p1, &p2);

    return 0;
}
