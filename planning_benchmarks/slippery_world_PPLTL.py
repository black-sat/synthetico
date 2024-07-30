# amount of columns and rows. Total amount of locations is gridsize^2

# gridsize can be altered to increase the domain. all variables are generated based on this
gridsize = 9


def generate_locations(size):
    rows = []
    cols = []

    for i in range(size):
        rows.append("r" + str(i + 1))
        cols.append("c" + str(i + 1))

    return rows, cols


rows, cols = generate_locations(gridsize)
agent_actions = ["l", "r", "u", "d"]
### partition file:
# print out partition file
print(".inputs: ", " ".join(rows + cols), sep='')
print(".outputs: ", " ".join(agent_actions), sep='')


### mutual exclusion for actions/fluents + agent preconditions
def get_mutual_exclusion_actions(actions):
    all_actions_or = " | ".join(actions)
    action_combinations = []

    for i, action in enumerate(actions):
        for j in range(i + 1, len(actions)):
            action_combinations.append("!(" + actions[i] + " & " + actions[j] + ")")

    action_comb_str = " & ".join(action_combinations)

    return all_actions_or, action_comb_str


def get_agent_preconditions():
    prec_list = []

    prec_list.append("(l -> !(" + cols[0] + "))")
    prec_list.append("(r -> !(" + cols[-1] + "))")
    prec_list.append("(u -> !(" + rows[0] + "))")
    prec_list.append("(d -> !(" + rows[-1] + "))")

    return prec_list

def get_agent_preconditions_PPLTL():
    prec_dictionary = {}

    prec_dictionary["l"] = "!(" + cols[0] + ")"
    prec_dictionary["r"] = "!(" + cols[-1] + ")"
    prec_dictionary["u"] = "!(" + rows[0] + ")"
    prec_dictionary["d"] = "!(" + rows[-1] + ")"

    return prec_dictionary


### environment transitions (also states that remain the same)
def get_environment_transitions():
    trans_list = []

    # action left
    for i, col in enumerate(cols):
        if i == 0:
            pass
        elif i - 1 == 0:
            trans_list.append("(" + col + " -> X(l -> " + cols[i - 1] + "))")
        else:
            trans_list.append("(" + col + " -> X(l -> (" + cols[i - 1] + " | " + cols[i - 2] + ")))")

    # action right
    for i, col in enumerate(cols):
        if i == len(cols) - 1:
            pass
        elif i == len(cols) - 2:
            trans_list.append("(" + col + " -> X(r -> " + cols[i + 1] + "))")
        else:
            trans_list.append("(" + col + " -> X(r -> (" + cols[i + 1] + " | " + cols[i + 2] + ")))")

    # action up
    for i, row in enumerate(rows):
        if i == 0:
            pass
        elif i - 1 == 0:
            trans_list.append("(" + row + " -> X(u -> " + rows[i - 1] + "))")
        else:
            trans_list.append("(" + row + " -> X(u -> (" + rows[i - 1] + " | " + rows[i - 2] + ")))")

    # action down
    for i, row in enumerate(rows):
        if i == len(rows) - 1:
            pass
        elif i == len(rows) - 2:
            trans_list.append("(" + row + " -> X(d -> " + rows[i + 1] + "))")
        else:
            trans_list.append("(" + row + " -> X(d -> (" + rows[i + 1] + " | " + rows[i + 2] + ")))")

    return trans_list

def get_environment_transitions_PPLTL():
    trans_list = []
    prec_dictionary = get_agent_preconditions_PPLTL()
    # action left
    for i, col in enumerate(cols):
        if i == 0:
            pass
        elif i - 1 == 0:
            trans_list.append("(Y(" + col + ") -> ((l & Y(" + prec_dictionary["l"] + ")) -> " + cols[i - 1] + "))")
        else:
            trans_list.append("(Y(" + col + ") -> ((l & Y(" + prec_dictionary["l"] + ")) -> (" + cols[i - 1] + " | " + cols[i - 2] + ")))")

    # action right
    for i, col in enumerate(cols):
        if i == len(cols) - 1:
            pass
        elif i == len(cols) - 2:
            trans_list.append("(Y(" + col + ") -> ((r & Y(" + prec_dictionary["r"] + ")) -> " + cols[i + 1] + "))")
        else:
            trans_list.append("(Y(" + col + ") -> ((r & Y(" + prec_dictionary["r"] + ")) -> (" + cols[i + 1] + " | " + cols[i + 2] + ")))")

    # action up
    for i, row in enumerate(rows):
        if i == 0:
            pass
        elif i - 1 == 0:
            trans_list.append("(Y(" + row + ") -> ((u & Y(" + prec_dictionary["u"] + ")) -> " + rows[i - 1] + "))")
        else:
            trans_list.append("(Y(" + row + ") -> ((u & Y(" + prec_dictionary["u"] + ")) -> (" + rows[i - 1] + " | " + rows[i - 2] + ")))")

    # action down
    for i, row in enumerate(rows):
        if i == len(rows) - 1:
            pass
        elif i == len(rows) - 2:
            trans_list.append("(Y(" + row + ") -> ((d & Y(" + prec_dictionary["d"] + ")) -> " + rows[i + 1] + "))")
        else:
            trans_list.append("(Y(" + row + ") -> ((d & Y(" + prec_dictionary["d"] + ")) -> (" + rows[i + 1] + " | " + rows[i + 2] + ")))")

    return trans_list


def get_same_next_state_PPLTL():
    next_list = []
    prec_dictionary = get_agent_preconditions_PPLTL()

    # stay in same column when going up or down
    for col in cols:
        next_list.append("(Y(" + col + ") -> ((u & Y(" + prec_dictionary["u"] + ")) -> " + col + "))")
        next_list.append("(Y(" + col + ") -> ((d & Y(" + prec_dictionary["d"] + ")) -> " + col + "))")

    # stay in same row when going left or right
    for row in rows:
        next_list.append("(Y(" + row + ") -> ((l & Y(" + prec_dictionary["l"] + ")) -> " + row + "))")
        next_list.append("(Y(" + row + ") -> ((r & Y(" + prec_dictionary["r"] + ")) -> " + row + "))")

    return next_list


# env_transitions = get_environment_transitions() + get_same_next_state()
env_transitions_PPLTL = get_environment_transitions_PPLTL() + get_same_next_state_PPLTL()

# TODO: check if same next state can be removed
# env_transitions = get_environment_transitions()
### set init, goal, and game-over states below by hand (hardcoded):
def get_init():
    return ["r1", "c1"]


def get_goal():
    return ["r4", "c4"]


def get_game_over_states():
    return ["r3", "c2"]


# get mutual exclusion for agent actions and for environment rows + cols (can be only in 1 row and 1 column at a time)
mut_exc_agent_all, mut_exc_agent_comb = get_mutual_exclusion_actions(agent_actions)
mut_exc_env_rows_all, mutual_excl_env_rows_comb = get_mutual_exclusion_actions(rows)
mut_exc_env_cols_all, mutual_excl_env_cols_comb = get_mutual_exclusion_actions(cols)
## Below are outputs in separate parts (init, agent, environment, and goal)
# init
print("(", " & ".join(get_init()), ")", sep='')
# agent
print("((", mut_exc_agent_all, ") & ", mut_exc_agent_comb, ")", sep='')
# environment
print("(((", mut_exc_env_rows_all, ") & ", mutual_excl_env_rows_comb, ") & ((", mut_exc_env_cols_all, ") & ",
      mutual_excl_env_cols_comb, ") & (", " & ".join(env_transitions_PPLTL), "))", sep='')
# preconditions agent actions
# print("(G((X(", cols[0], " | !", cols[0], ")) -> (", " & ".join(get_agent_preconditions()), ")))", sep="")
# print("(H(", " & ".join(get_agent_preconditions()), "))", sep="")
# goal
print("F(", " & ".join(get_goal()), ")", sep='')


# complete formula, written as follows: (H(phi_agent_unique_act) & ((phi_env_init & H(phi_env_transitions_PPLTL)) -> (phi_goal)))
print("PPLTL formula:\n", "F((H((", mut_exc_agent_all, ") & (", mut_exc_agent_comb, ")) & (", " & ".join(get_init()),
      ") & H((", mut_exc_env_rows_all, ") & (", mutual_excl_env_rows_comb, ") & (", mut_exc_env_cols_all, ") & (",
      mutual_excl_env_cols_comb, ") & (", " & ".join(env_transitions_PPLTL), "))) -> (O(", " & ".join(get_goal()), ")))", sep='')

# print out partition file
print(".inputs: ", " ".join(rows + cols), sep='')
print(".outputs: ", " ".join(agent_actions), sep='')