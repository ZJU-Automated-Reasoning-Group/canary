/*
 * Author: rainoftime
 * File Description: Peephole optimization for SMT simplifications
 * Creation Date:  2023.
 * Modification History:
 */

#pragma once

#include <z3++.h>
#include <z3.h>


z3::expr peephole_simplify_expr_vector(const z3::expr_vector &);

z3::expr peephole_simplify_expr(const z3::expr &);
