public class Echo {
	public static void main(String[] args) {
		for (int i = 0; i < args.length; i++) {
			System.out.print(args[i]);
			if (i + 1 == args.length) {
				System.out.println();
			} else {
				System.out.print(' ');
			}
		}
	}
}
