main :: (arguments : [] string) -> i32
{
	print("Hello\n");
	foreach (argument in arguments) {
		printf("%s\n", argument);
	}
	return 0;
}
