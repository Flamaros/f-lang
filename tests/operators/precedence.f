// - need parenthesis for 3 * 5 / 4, because of the ambiguity, the order is important here
// 
// Il faut checker quand il y a ambuiguité quand les opérateurs ont la même priorité et qu'ils ne sont pas commutatif. 4 + 5 - 1 ne doit pas demander des parenthèses

x: i32 = 5 * 3 + 4;
